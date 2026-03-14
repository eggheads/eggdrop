#[allow(
    non_upper_case_globals,
    non_camel_case_types,
    non_snake_case,
    dead_code
)]
pub mod eggdrop {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

use std::ffi::{CStr, CString, c_char, c_int};
use std::fs::File;
use std::io::{BufRead, BufReader, LineWriter, Write as IoWrite};
use std::os::unix::io::FromRawFd;
use std::sync::{Mutex, OnceLock};
use std::time::Duration;
use tempfile::NamedTempFile;

use eggdrop::{add_hook, sockname_t, HOOK_DNS_HOSTBYIP, HOOK_DNS_IPBYHOST};

unsafe extern "C" {
    fn eggdrop_main(argc: c_int, argv: *mut *mut c_char) -> c_int;
    fn call_ipbyhost(hostn: *mut c_char, ip: *mut sockname_t, ok: c_int);
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
            let ip = u32::from_be(sa.sin_addr.s_addr);
            format!(
                "{}.{}.{}.{}",
                (ip >> 24) & 0xff,
                (ip >> 16) & 0xff,
                (ip >> 8) & 0xff,
                ip & 0xff
            )
        }
        libc::AF_INET6 => {
            let sa = unsafe { &(*addr).addr.s6 };
            let bytes = unsafe { sa.sin6_addr.__in6_u.__u6_addr8 };
            let segments: Vec<String> = (0..8)
                .map(|i| format!("{:x}", u16::from_be_bytes([bytes[i * 2], bytes[i * 2 + 1]])))
                .collect();
            segments.join(":")
        }
        _ => format!("unknown family {}", family),
    };
    eprintln!("[eggtest dns] reverse lookup for: {}", ip_str);
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
        assert!(
            !ptr.is_null(),
            "dlsym(RTLD_NEXT, \"connect\") returned NULL"
        );
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

/// Our end of the fake IRC server socketpair fd (-1 = not connected yet)
static FAKE_IRCD_FD: Mutex<c_int> = Mutex::new(-1);

#[unsafe(no_mangle)]
unsafe extern "C" fn connect(
    fd: c_int,
    addr: *const libc::sockaddr,
    len: libc::socklen_t,
) -> c_int {
    if is_fake_target(addr) {
        let mut pair: [c_int; 2] = [0; 2];
        if unsafe { libc::socketpair(libc::AF_UNIX, libc::SOCK_STREAM, 0, pair.as_mut_ptr()) } != 0
        {
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
        eprintln!(
            "[eggtest connect] intercepted fd={} <-> rust fd={}",
            fd, pair[1]
        );
        *FAKE_IRCD_FD.lock().unwrap() = pair[1];
        return 0;
    }

    let real_connect = get_real_connect();
    unsafe { real_connect(fd, addr, len) }
}

/// Handle to the fake IRC server side of the socketpair.
pub struct IRCd {
    reader: BufReader<File>,
    writer: LineWriter<File>,
    isupport: Vec<String>,
}

/// A parsed IRC line, split into words on whitespace.
#[derive(Debug, Clone)]
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

    /// Check if all words match exactly (same length and content).
    pub fn matches(&self, other: &IRCline) -> bool {
        self.0.len() == other.0.len() && self.0 == other.0
    }
}

impl std::fmt::Display for IRCline {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let quoted: Vec<String> = self.0.iter().map(|w| format!("\"{}\"", w)).collect();
        write!(f, "{}", quoted.join(" "))
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

    /// Read lines until one matches the given string exactly (parsed as IRCline), panic on timeout.
    pub fn expect(&mut self, expected: &str, timeout: Duration) {
        let expected = IRCline::new(expected);
        let deadline = std::time::Instant::now() + timeout;
        loop {
            let remaining = deadline.saturating_duration_since(std::time::Instant::now());
            if remaining.is_zero() {
                panic!("IRCd::expect({}) timed out", expected);
            }
            if let Some(line) = self.recv_line(remaining) {
                eprintln!("[eggtest ircd] <- {}", line);
                if line.matches(&expected) {
                    return;
                }
            } else {
                panic!("IRCd::expect({}) timed out", expected);
            }
        }
    }

    /// Read lines until one starts with the given prefix (parsed as IRCline), panic on timeout.
    pub fn expect_prefix(&mut self, prefix: &str, timeout: Duration) -> IRCline {
        let prefix = IRCline::new(prefix);
        let deadline = std::time::Instant::now() + timeout;
        loop {
            let remaining = deadline.saturating_duration_since(std::time::Instant::now());
            if remaining.is_zero() {
                panic!("IRCd::expect_prefix({}) timed out", prefix);
            }
            if let Some(line) = self.recv_line(remaining) {
                eprintln!("[eggtest ircd] <- {}", line);
                if line.starts_with(&prefix) {
                    return line;
                }
            } else {
                panic!("IRCd::expect_prefix({}) timed out", prefix);
            }
        }
    }

    /// Drain all pending lines (useful to skip past startup chatter).
    pub fn drain(&mut self, quiet_period: Duration) {
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
        self.expect_prefix("CAP LS", Duration::from_secs(10));
        self.send(":irc.test CAP * LS :");
        let nick_line = self.expect_prefix("NICK", Duration::from_secs(5));
        self.expect_prefix("USER", Duration::from_secs(5));
        self.expect("CAP END", Duration::from_secs(5));
        nick_line[1].clone()
    }

    /// Send a standard IRC welcome burst (001-005).
    pub fn send_welcome(&mut self, nick: &str) {
        self.send(&format!(
            ":irc.test 001 {} :Welcome to the test network",
            nick
        ));
        self.send(&format!(":irc.test 002 {} :Your host is irc.test", nick));
        self.send(&format!(
            ":irc.test 003 {} :This server was created today",
            nick
        ));
        self.send(&format!(
            ":irc.test 004 {} irc.test test-0.1 oiwszcrkfydnxbauglZCD biklmnopstveIrS bkloveI",
            nick
        ));
        let isupport = self.isupport.clone();
        for tokens in &isupport {
            self.send(&format!(
                ":irc.test 005 {} {} :are supported by this server",
                nick, tokens
            ));
        }
    }
}

const DEFAULT_ISUPPORT: &[&str] = &["NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#&"];

/// Config builder for Eggtest.
pub struct EggtestBuilder {
    settings: Vec<(String, String)>,
    isupport: Vec<String>,
}

/// The main test handle. Created via `Eggtest::builder().spawn()`.
pub struct Eggtest {
    pub ircd: IRCd,
    _conffile: NamedTempFile,
    _pidfile: NamedTempFile,
}

impl Eggtest {
    /// Create a builder with default settings.
    pub fn builder() -> EggtestBuilder {
        EggtestBuilder {
            settings: Vec::new(),
            isupport: DEFAULT_ISUPPORT.iter().map(|s| s.to_string()).collect(),
        }
    }

    /// Shorthand: spawn with default settings.
    pub fn spawn() -> Eggtest {
        Eggtest::builder().spawn()
    }
}

/// Default config template, loaded from eggdrop.conf.j2 at compile time.
const CONFIG_TEMPLATE: &str = include_str!("../eggdrop.conf.j2");

impl EggtestBuilder {
    /// Set custom raw 005 (ISUPPORT) lines for the fake IRCd welcome burst.
    /// Each entry becomes a separate 005 numeric line.
    pub fn with_isupport(mut self, lines: Vec<&str>) -> Self {
        self.isupport = lines.into_iter().map(|s| s.to_string()).collect();
        self
    }

    /// Add or override a setting. Will be emitted as `set <setting> "<value>"`.
    pub fn with_set(mut self, setting: &str, value: &str) -> Self {
        self.settings.push((setting.to_string(), value.to_string()));
        self
    }

    /// Build the config, start eggdrop, wait for IRC connection.
    pub fn spawn(self) -> Eggtest {
        // Render config template
        let mut conffile = NamedTempFile::new().expect("create temp config file");
        let pidfile = NamedTempFile::new().expect("create temp pidfile");
        let pidfile_path = pidfile.path().to_str().expect("pidfile path").to_string();

        let mut env = minijinja::Environment::new();
        env.add_template("config", CONFIG_TEMPLATE)
            .expect("parse config template");
        let tmpl = env.get_template("config").unwrap();
        let config = tmpl
            .render(minijinja::context! {
                pidfile => pidfile_path,
                settings => self.settings,
            })
            .expect("render config template");
        write!(conffile, "{}", config).expect("write config");
        conffile.flush().expect("flush config");

        // Install DNS hooks
        unsafe {
            add_hook(HOOK_DNS_HOSTBYIP as c_int, std::mem::transmute(rust_dns_hostbyip as *const ()));
            add_hook(HOOK_DNS_IPBYHOST as c_int, std::mem::transmute(rust_dns_ipbyhost as *const ()));
        }

        let flags = if std::path::Path::new("LamestBot.user").exists() {
            "-n"
        } else {
            "-mn"
        };

        let conf_path = conffile.path().to_str().expect("tmpfile path").to_string();
        let args: &[CString] = Box::leak(Box::new([
            CString::new("./eggdrop").unwrap(),
            CString::new(flags).unwrap(),
            CString::new(conf_path).unwrap(),
        ]));
        let mut argv: Vec<*mut c_char> = args.iter().map(|a| a.as_ptr() as *mut _).collect();
        argv.push(std::ptr::null_mut());
        let argv_ptr = argv.as_mut_ptr() as usize;

        std::thread::spawn(move || unsafe {
            eggdrop_main(3, argv_ptr as *mut *mut c_char);
        });

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

        Eggtest {
            ircd: IRCd {
                reader: BufReader::new(read_file),
                writer: LineWriter::new(write_file),
                isupport: self.isupport,
            },
            _conffile: conffile,
            _pidfile: pidfile,
        }
    }
}
