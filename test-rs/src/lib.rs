#[allow(non_upper_case_globals, non_camel_case_types, non_snake_case, dead_code)]
pub mod eggdrop {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

/// Read a `static mut` without creating a reference (Rust 2024 forbids `&static_mut`).
#[macro_export]
macro_rules! read_static {
    ($sym:expr) => {
        unsafe { std::ptr::addr_of!($sym).read() }
    };
}

use std::ffi::{CStr, CString, c_char, c_int};
use std::fs::File;
use std::io::{BufRead, BufReader, LineWriter, Write as IoWrite};
use std::net::{Ipv4Addr, Ipv6Addr, TcpStream};
use std::os::unix::io::FromRawFd;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Mutex, OnceLock};
use std::time::Duration;
use tempfile::TempDir;

use eggdrop::{HOOK_DNS_HOSTBYIP, HOOK_DNS_IPBYHOST, add_hook, sockname_t};

unsafe extern "C" {
    fn eggdrop_main(argc: c_int, argv: *mut *mut c_char) -> c_int;
    fn call_ipbyhost(hostn: *mut c_char, ip: *mut sockname_t, ok: c_int);
    fn call_hostbyip(ip: *mut sockname_t, hostn: *mut c_char, ok: c_int);
    pub fn isupport_get(key: *const c_char, keylen: usize) -> *const c_char;
    pub fn extban_parse(mask: *const c_char, typ: *mut c_char, arg: *mut *const c_char) -> c_int;
    pub fn extban_flag_supported_local(flag: c_char) -> c_int;
    pub fn is_extban_mask(mask: *const c_char) -> c_int;
    pub fn get_extban_prefix(prefix: *mut c_char);
    pub fn extban_is_enforceable_flag(flag: c_char, account_extban_flag: c_char) -> c_int;
    pub fn banmask_matches_member(banmask: *const c_char, user: *const c_char, m: *mut eggdrop::memberlist) -> c_int;
}

/// Build a sockname_t containing an IPv4 address
fn make_sockname_v4(a: u8, b: u8, c: u8, d: u8) -> sockname_t {
    let mut sn = sockname_t::default();
    sn.family = libc::AF_INET;
    sn.addrlen = std::mem::size_of::<libc::sockaddr_in>() as u32;
    let sa = unsafe { &mut sn.addr.s4 };
    sa.sin_family = libc::AF_INET as libc::sa_family_t;
    sa.sin_addr.s_addr = u32::from_ne_bytes([a, b, c, d]);
    sn
}

unsafe extern "C" fn rust_dns_hostbyip(addr: *mut sockname_t) {
    let family = unsafe { (*addr).family };
    let ip_str = match family {
        libc::AF_INET => {
            let sa = unsafe { &(*addr).addr.s4 };
            Ipv4Addr::from(sa.sin_addr.s_addr.to_ne_bytes()).to_string()
        }
        libc::AF_INET6 => {
            let bytes: [u8; 16] = unsafe { (*addr).addr.s6.sin6_addr.__in6_u.__u6_addr8 };
            Ipv6Addr::from(bytes).to_string()
        }
        _ => format!("unknown family {}", family),
    };
    eprintln!("[eggtest dns] reverse lookup for: {}", ip_str);

    // Resolve all reverse lookups to the IP string itself
    let hostname = CString::new(ip_str).unwrap();
    unsafe {
        call_hostbyip(addr, hostname.as_ptr() as *mut c_char, 1);
    }
}

unsafe extern "C" fn rust_dns_ipbyhost(hostname: *mut c_char) {
    let name = unsafe { CStr::from_ptr(hostname) }.to_string_lossy();
    eprintln!("[eggtest dns] forward lookup for: {}", name);

    if *name == *"fake.test-rs" {
        eprintln!("[eggtest dns] resolving fake.test-rs -> 192.0.2.1");
        let mut sn = make_sockname_v4(192, 0, 2, 1);
        unsafe {
            call_ipbyhost(hostname, &mut sn, 1);
        }
    }
}

type ConnectFn = unsafe extern "C" fn(c_int, *const libc::sockaddr, libc::socklen_t) -> c_int;

static REAL_CONNECT: OnceLock<ConnectFn> = OnceLock::new();

fn get_real_connect() -> ConnectFn {
    *REAL_CONNECT.get_or_init(|| unsafe {
        let ptr = libc::dlsym(libc::RTLD_NEXT, c"connect".as_ptr());
        assert!(!ptr.is_null(), "dlsym(RTLD_NEXT, \"connect\") returned NULL");
        std::mem::transmute(ptr)
    })
}

fn is_fake_target(addr: *const libc::sockaddr) -> bool {
    let family = unsafe { (*addr).sa_family } as c_int;
    if family != libc::AF_INET {
        return false;
    }
    let sa = unsafe { &*(addr as *const libc::sockaddr_in) };
    let ip_bytes = sa.sin_addr.s_addr.to_ne_bytes();
    let port = u16::from_be(sa.sin_port);
    ip_bytes == [192, 0, 2, 1] && port == 6667
}

/// Whether the connect() interposition should intercept connections to the fake server.
static INTERCEPT_CONNECT: AtomicBool = AtomicBool::new(false);

/// Our end of the fake IRC server socketpair fd (-1 = not connected yet)
static FAKE_IRCD_FD: Mutex<c_int> = Mutex::new(-1);

#[unsafe(no_mangle)]
unsafe extern "C" fn connect(fd: c_int, addr: *const libc::sockaddr, len: libc::socklen_t) -> c_int {
    if INTERCEPT_CONNECT.load(Ordering::Relaxed) && is_fake_target(addr) {
        let mut pair: [c_int; 2] = [0; 2];
        if unsafe { libc::socketpair(libc::AF_UNIX, libc::SOCK_STREAM, 0, pair.as_mut_ptr()) } != 0 {
            eprintln!("[eggtest connect] socketpair() failed");
            unsafe {
                *libc::__errno_location() = libc::ECONNREFUSED;
            }
            return -1;
        }
        unsafe {
            libc::dup2(pair[0], fd);
            libc::close(pair[0]);
        }
        eprintln!("[eggtest connect] intercepted fd={} <-> rust fd={}", fd, pair[1]);
        *FAKE_IRCD_FD.lock().unwrap() = pair[1];
        return 0;
    }

    let real_connect = get_real_connect();
    unsafe { real_connect(fd, addr, len) }
}

/// A member in a simulated channel for join_chan().
pub struct ChannelMember<'a> {
    pub nick: &'a str,
    pub user: &'a str,
    pub host: &'a str,
    pub flags: &'a str,   // WHO flags: "H" (here), "H@" (here+op), "H+" (here+voice)
    pub account: &'a str, // Account name, "*" if not logged in
}

/// Handle to the fake IRC server side of the socketpair.
pub struct IRCd {
    reader: BufReader<File>,
    writer: LineWriter<File>,
    isupport: Vec<String>,
}

/// A parsed IRC line, split into words on whitespace.
#[derive(Debug, Clone, PartialEq)]
pub struct IRCline(pub Vec<String>);

impl IRCline {
    pub fn new(raw: &str) -> Self {
        let mut words = Vec::new();
        let mut rest = raw;
        while !rest.is_empty() {
            rest = rest.trim_start();
            if rest.is_empty() {
                break;
            }
            if rest.starts_with(':') && !words.is_empty() {
                // Trailing parameter: everything after the ':' is one word
                words.push(rest[1..].to_string());
                break;
            }
            match rest.find(' ') {
                Some(i) => {
                    words.push(rest[..i].to_string());
                    rest = &rest[i..];
                }
                None => {
                    words.push(rest.to_string());
                    break;
                }
            }
        }
        IRCline(words)
    }

    /// Access a word by index.
    pub fn get(&self, i: usize) -> Option<&str> {
        self.0.get(i).map(|s| s.as_str())
    }

    /// Check if the first N words match another IRCline's words.
    pub fn starts_with(&self, other: &IRCline) -> bool {
        if other.0.len() > self.0.len() {
            return false;
        }
        other.0.iter().zip(&self.0).all(|(a, b)| a == b)
    }
}

impl std::fmt::Display for IRCline {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for (i, w) in self.0.iter().enumerate() {
            if i > 0 {
                f.write_str(" ")?;
            }
            write!(f, "\"{}\"", w)?;
        }
        Ok(())
    }
}

impl std::ops::Index<usize> for IRCline {
    type Output = String;
    fn index(&self, i: usize) -> &String {
        &self.0[i]
    }
}

impl IRCd {
    /// Send a raw IRC line (appends \r\n).
    pub fn send(&mut self, line: &str) {
        eprintln!("[eggtest ircd] -> {line:?}");
        write!(self.writer, "{}\r\n", line).expect("write to fake ircd socket");
    }

    /// Read one IRC line, stripping \r\n. Returns None on timeout.
    pub fn recv_line(&mut self, timeout: Duration) -> Option<IRCline> {
        use std::os::unix::io::AsRawFd;
        let fd = self.reader.get_ref().as_raw_fd();

        let mut pfd = libc::pollfd {
            fd,
            events: libc::POLLIN,
            revents: 0,
        };
        let ms = timeout.as_millis() as c_int;
        let ret = unsafe { libc::poll(&mut pfd, 1, ms) };
        if ret <= 0 {
            return None;
        }

        let mut line = String::new();
        match self.reader.read_line(&mut line) {
            Ok(0) | Err(_) => None,
            Ok(_) => {
                let trimmed = line.trim_end_matches(&['\r', '\n'][..]);
                Some(IRCline::new(trimmed))
            }
        }
    }

    /// Read lines until `pred` matches, panic on timeout. Returns the matched line.
    fn recv_until(&mut self, label: &impl std::fmt::Display, timeout: Duration, pred: impl Fn(&IRCline) -> bool) -> IRCline {
        let deadline = std::time::Instant::now() + timeout;
        loop {
            let remaining = deadline.saturating_duration_since(std::time::Instant::now());
            if remaining.is_zero() {
                panic!("IRCd::{} timed out", label);
            }
            if let Some(line) = self.recv_line(remaining) {
                eprintln!("[eggtest ircd] <- {}", line);
                if pred(&line) {
                    return line;
                }
            } else {
                panic!("IRCd::{} timed out", label);
            }
        }
    }

    /// Read lines until one matches the given string exactly (parsed as IRCline), panic on timeout.
    /// Timeout is a string like "5s", "500ms", "10s".
    pub fn expect(&mut self, expected: &str, timeout: &str) {
        let timeout = parse_duration(timeout);
        let expected = IRCline::new(expected);
        self.recv_until(&format_args!("expect({})", expected), timeout, |line| *line == expected);
    }

    /// Read lines until one starts with the given prefix (parsed as IRCline), panic on timeout.
    /// Timeout is a string like "5s", "500ms", "10s".
    pub fn expect_prefix(&mut self, prefix: &str, timeout: &str) -> IRCline {
        let timeout = parse_duration(timeout);
        let prefix = IRCline::new(prefix);
        self.recv_until(&format_args!("expect_prefix({})", prefix), timeout, |line| line.starts_with(&prefix))
    }

    /// Drain all pending lines (useful to skip past startup chatter).
    /// Quiet period is a string like "500ms", "1s".
    pub fn drain(&mut self, quiet_period: &str) {
        let quiet_period = parse_duration(quiet_period);
        loop {
            match self.recv_line(quiet_period) {
                Some(line) => eprintln!("[eggtest ircd] drain: {}", line),
                None => break,
            }
        }
    }

    /// Handle CAP LS negotiation (empty cap list), wait for NICK/USER,
    /// return the nick eggdrop registered with.
    pub fn negotiate(&mut self) -> String {
        self.expect_prefix("CAP LS", "10s");
        self.send(":irc.test CAP * LS :");
        let nick_line = self.expect_prefix("NICK", "5s");
        self.expect_prefix("USER", "5s");
        self.expect("CAP END", "5s");
        nick_line[1].clone()
    }

    /// Simulate eggdrop joining a channel: handle the full JOIN/NAMES/MODE/WHO sequence.
    /// `botnick` is the bot's nick, `botuser` is the bot's username, `bothost` is the bot's host.
    /// `chan` is the channel name. `members` lists other channel members (the bot is added
    /// automatically to NAMES and WHO). `bans` are raw 367 ban entries
    /// (e.g. `"*!*@evil.com setter 1700000000"`).
    pub fn join_chan(&mut self, nick: &str, user: &str, host: &str, chan: &str, members: &[ChannelMember], bans: &[&str]) {
        // 1. expect JOIN from bot
        self.expect(&format!("JOIN {chan}"), "10s");

        // 2. send JOIN echo, NAMES (353), end of NAMES (366)
        self.send(&format!(":{nick}!{user}@{host} JOIN :{chan}"));
        let mut names = nick.to_string();
        for m in members {
            names.push(' ');
            if m.flags.contains('@') {
                names.push('@');
            } else if m.flags.contains('+') {
                names.push('+');
            }
            names.push_str(m.nick);
        }
        self.send(&format!(":irc.test 353 {nick} = {chan} :{names}"));
        self.send(&format!(":irc.test 366 {nick} {chan} :End of /NAMES list."));

        // 3. expect MODE +b, send ban list + end
        self.expect(&format!("MODE {chan} +b"), "5s");
        for ban in bans {
            self.send(&format!(":irc.test 367 {nick} {chan} {ban}"));
        }
        self.send(&format!(":irc.test 368 {nick} {chan} :End of Channel Ban List"));

        // 4. expect MODE +e, send end of exception list
        self.expect(&format!("MODE {chan} +e"), "5s");
        self.send(&format!(":irc.test 349 {nick} {chan} :End of Channel Exception List"));

        // 5. expect MODE +I, send end of invite list
        self.expect(&format!("MODE {chan} +I"), "5s");
        self.send(&format!(":irc.test 347 {nick} {chan} :End of Channel Invite List"));

        // 6. expect MODE #chan (channel modes query), send 324 + 329
        self.expect(&format!("MODE {chan}"), "5s");
        self.send(&format!(":irc.test 324 {nick} {chan} +"));
        self.send(&format!(":irc.test 329 {nick} {chan} 1700000000"));

        // 7. expect WHO (WHOX: WHO #chan c%chnufat,222), send 354 entries + 315 end
        self.expect_prefix(&format!("WHO {chan}"), "10s");
        // Bot's own WHO entry
        self.send(&format!(":irc.test 354 {nick} 222 {chan} {user} {host} {nick} H :0 eggdrop"));
        // Other members
        for m in members {
            let mnick = m.nick;
            let muser = m.user;
            let mhost = m.host;
            let mflags = m.flags;
            let maccount = m.account;
            self.send(&format!(":irc.test 354 {nick} 222 {chan} {muser} {mhost} {mnick} {mflags} {maccount}"));
        }
        self.send(&format!(":irc.test 315 {nick} {chan} :End of /WHO list."));
    }

    /// Send a standard IRC welcome burst (001-005).
    pub fn send_welcome(&mut self, nick: &str) {
        self.send(&format!(":irc.test 001 {} :Welcome to the test network", nick));
        self.send(&format!(":irc.test 002 {} :Your host is irc.test", nick));
        self.send(&format!(":irc.test 003 {} :This server was created today", nick));
        self.send(&format!(
            ":irc.test 004 {} irc.test test-0.1 oiwszcrkfydnxbauglZCD biklmnopstveIrS bkloveI",
            nick
        ));
        for i in 0..self.isupport.len() {
            let line = format!(":irc.test 005 {} {} :are supported by this server", nick, &self.isupport[i]);
            self.send(&line);
        }
    }
}

const DEFAULT_ISUPPORT: &[&str] = &["NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#&"];

const DEFAULT_MODULES: &[&str] = &["pbkdf2", "blowfish", "channels", "server", "ctcp", "irc", "notes", "console"];

/// Config builder for Eggtest.
pub struct EggtestBuilder {
    settings: Vec<(String, String)>,
    modules: Vec<String>,
    isupport: Vec<String>,
    channels: Vec<String>,
    tcl_extra: Vec<String>,
    use_ircd: bool,
}

/// The main test handle. Created via `Eggtest::builder().spawn()`.
pub struct Eggtest {
    pub ircd: IRCd,
    tcl_reader: BufReader<TcpStream>,
    tcl_writer: TcpStream,
    _workdir: TempDir,
}

impl Eggtest {
    /// Create a builder with default settings.
    pub fn builder() -> EggtestBuilder {
        EggtestBuilder {
            settings: Vec::new(),
            modules: DEFAULT_MODULES.iter().map(|s| s.to_string()).collect(),
            isupport: DEFAULT_ISUPPORT.iter().map(|s| s.to_string()).collect(),
            channels: Vec::new(),
            tcl_extra: Vec::new(),
            use_ircd: true,
        }
    }

    /// Shorthand: spawn with default settings.
    pub fn spawn() -> Eggtest {
        Eggtest::builder().spawn()
    }

    /// Evaluate a Tcl script in eggdrop's interpreter via the eval server socket.
    /// Runs on eggdrop's main thread (thread-safe). Returns the Tcl result string.
    /// Panics on Tcl errors.
    pub fn tcl(&mut self, script: &str) -> String {
        IoWrite::write_fmt(&mut self.tcl_writer, format_args!("{}\n", script)).expect("write to tcl eval socket");
        IoWrite::flush(&mut self.tcl_writer).expect("flush tcl eval socket");
        let mut line = String::new();
        self.tcl_reader.read_line(&mut line).expect("read from tcl eval socket");
        let line = line.trim_end_matches(&['\r', '\n'][..]);
        match line.split_once(' ') {
            Some(("0", result)) => result.to_string(),
            Some((code, result)) => panic!("Tcl error (code {code}): {result}"),
            None if line == "0" => String::new(),
            None => panic!("unexpected tcl eval response: {line:?}"),
        }
    }
}

/// Default config template, loaded from eggdrop.conf.j2 at compile time.
const CONFIG_TEMPLATE: &str = include_str!("../eggdrop.conf.j2");

impl EggtestBuilder {
    /// Set the list of modules to load. Replaces the default module list.
    pub fn with_modules(mut self, modules: Vec<&str>) -> Self {
        self.modules = modules.into_iter().map(|s| s.to_string()).collect();
        self
    }

    /// Set custom raw 005 (ISUPPORT) lines for the fake IRCd welcome burst.
    /// Each entry becomes a separate 005 numeric line.
    pub fn with_isupport(mut self, lines: Vec<&str>) -> Self {
        self.isupport = lines.into_iter().map(|s| s.to_string()).collect();
        self
    }

    /// Add channels to the config. Each entry is a channel name (e.g. "#test").
    pub fn with_channels(mut self, channels: Vec<&str>) -> Self {
        self.channels = channels.into_iter().map(|s| s.to_string()).collect();
        self
    }

    /// Enable or disable the fake IRC server (DNS hooks, connect interception).
    /// Default is true.
    pub fn with_ircd(mut self, enabled: bool) -> Self {
        self.use_ircd = enabled;
        self
    }

    /// Add or override a setting. Will be emitted as `set <setting> "<value>"`.
    pub fn with_set(mut self, setting: &str, value: &str) -> Self {
        self.settings.push((setting.to_string(), value.to_string()));
        self
    }

    /// Build the config, start eggdrop, wait for IRC connection.
    pub fn spawn(self) -> Eggtest {
        let eggdrop_dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap();

        // Pick a free port for the Tcl eval server
        let tcl_eval_port = {
            let listener = std::net::TcpListener::bind("127.0.0.1:0").expect("bind to find free port");
            listener.local_addr().unwrap().port()
        };

        // Create a temporary working directory with symlinks to eggdrop's runtime dirs
        let workdir = TempDir::new().expect("create temp workdir");
        for name in &["help", "language", "modules", "scripts", "text"] {
            std::os::unix::fs::symlink(eggdrop_dir.join(name), workdir.path().join(name))
                .unwrap_or_else(|e| panic!("symlink {}: {}", name, e));
        }
        std::fs::create_dir(workdir.path().join("logs")).expect("create logs dir");

        // Render config as eggdrop.conf in the workdir (the default config name)
        let conf_path = workdir.path().join("eggdrop.conf");
        let mut env = minijinja::Environment::new();
        env.add_template("config", CONFIG_TEMPLATE).expect("parse config template");
        let tmpl = env.get_template("config").unwrap();
        let config = tmpl
            .render(minijinja::context! {
                modules => self.modules,
                settings => self.settings,
                channels => self.channels,
                tcl_extra => self.tcl_extra,
                tcl_eval_port => tcl_eval_port,
            })
            .expect("render config template");
        std::fs::write(&conf_path, &config).expect("write eggdrop.conf");

        if self.use_ircd {
            // Install DNS hooks and enable connect interception
            INTERCEPT_CONNECT.store(true, Ordering::Relaxed);
            *FAKE_IRCD_FD.lock().unwrap() = -1;
            unsafe {
                add_hook(HOOK_DNS_HOSTBYIP as c_int, std::mem::transmute(rust_dns_hostbyip as *const ()));
                add_hook(HOOK_DNS_IPBYHOST as c_int, std::mem::transmute(rust_dns_ipbyhost as *const ()));
            }
        }

        // No userfile yet → -mn (create one), otherwise -n (foreground only)
        let userfile = workdir.path().join("LamestBot.user");
        let flags = if userfile.exists() { "-n" } else { "-mn" };

        // Leaked intentionally: eggdrop_main holds argv for its lifetime (never returns).
        let args: &[CString] = Box::leak(Box::new([
            CString::new("./eggdrop").unwrap(),
            CString::new(flags).unwrap(),
        ]));
        let mut argv: Vec<*mut c_char> = args.iter().map(|a| a.as_ptr() as *mut _).collect();
        argv.push(std::ptr::null_mut());
        let argv: &mut [*mut c_char] = Box::leak(argv.into_boxed_slice());
        let argc = args.len() as c_int;
        let argv_ptr = argv.as_mut_ptr() as usize;

        let workdir_path = workdir.path().to_path_buf();
        std::thread::spawn(move || unsafe {
            std::env::set_current_dir(&workdir_path).expect("chdir to workdir");
            eggdrop_main(argc, argv_ptr as *mut *mut c_char);
        });

        let ircd = if self.use_ircd {
            // Wait for eggdrop to connect to fake.test-rs:6667
            let fd = loop {
                std::thread::sleep(Duration::from_millis(50));
                let fd = *FAKE_IRCD_FD.lock().unwrap();
                if fd >= 0 {
                    break fd;
                }
            };

            let read_file = unsafe { File::from_raw_fd(fd) };
            let write_file = read_file.try_clone().expect("dup ircd fd for writer");

            IRCd {
                reader: BufReader::new(read_file),
                writer: LineWriter::new(write_file),
                isupport: self.isupport,
            }
        } else {
            panic!("Eggtest::spawn() without ircd not yet supported — use with_ircd(true)")
        };

        // Connect to the Tcl eval server (listen is set up during config sourcing,
        // which completes before the IRC connection, so it's ready by now).
        let tcl_stream = TcpStream::connect(("127.0.0.1", tcl_eval_port))
            .expect("connect to tcl eval port");
        tcl_stream.set_read_timeout(Some(Duration::from_secs(10))).unwrap();
        let tcl_writer = tcl_stream.try_clone().expect("clone tcl stream");
        let tcl_reader = BufReader::new(tcl_stream);

        Eggtest {
            ircd,
            tcl_reader,
            tcl_writer,
            _workdir: workdir,
        }
    }
}

/// Parse a human-readable duration string like "5s", "500ms", "1500ms".
fn parse_duration(s: &str) -> Duration {
    if let Some(ms) = s.strip_suffix("ms") {
        Duration::from_millis(ms.parse().unwrap_or_else(|_| panic!("invalid duration: {:?}", s)))
    } else if let Some(secs) = s.strip_suffix('s') {
        Duration::from_secs(secs.parse().unwrap_or_else(|_| panic!("invalid duration: {:?}", s)))
    } else {
        panic!("invalid duration {:?}: expected suffix 's' or 'ms'", s)
    }
}
