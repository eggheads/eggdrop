use std::ffi::{CStr, CString, c_char, c_int, c_void};
use std::fs::File;
use std::io::{BufRead, BufReader, LineWriter, Write as IoWrite};
use std::os::unix::io::FromRawFd;
use std::path::Path;
use std::sync::{Mutex, OnceLock};
use std::time::Duration;

/// Eggdrop's sockname_t (with IPV6 enabled)
#[repr(C)]
struct SocknameTAddr {
    _data: [u8; std::mem::size_of::<libc::sockaddr_in6>()],
}

#[repr(C)]
struct SocknameT {
    family: c_int,
    addrlen: u32,
    addr: SocknameTAddr,
}

const HOOK_DNS_HOSTBYIP: c_int = 112;
const HOOK_DNS_IPBYHOST: c_int = 113;

unsafe extern "C" {
    fn eggdrop_main(argc: c_int, argv: *mut *mut c_char) -> c_int;
    fn add_hook(hook_num: c_int, func: *const c_void);
    fn call_ipbyhost(hostn: *mut c_char, ip: *mut SocknameT, ok: c_int);
}

/// Build a SocknameT containing an IPv4 address
fn make_sockname_v4(a: u8, b: u8, c: u8, d: u8) -> SocknameT {
    let mut sn = SocknameT {
        family: libc::AF_INET,
        addrlen: std::mem::size_of::<libc::sockaddr_in>() as u32,
        addr: SocknameTAddr {
            _data: [0u8; std::mem::size_of::<libc::sockaddr_in6>()],
        },
    };
    let sa = unsafe { &mut *(&raw mut sn.addr as *mut libc::sockaddr_in) };
    sa.sin_family = libc::AF_INET as libc::sa_family_t;
    sa.sin_addr.s_addr = u32::from_ne_bytes([a, b, c, d]);
    sn
}

unsafe extern "C" fn rust_dns_hostbyip(addr: *mut SocknameT) {
    let family = unsafe { (*addr).family };
    let ip_str = match family {
        libc::AF_INET => {
            let sa = unsafe { &*(std::ptr::addr_of!((*addr).addr) as *const libc::sockaddr_in) };
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
            let sa = unsafe { &*(std::ptr::addr_of!((*addr).addr) as *const libc::sockaddr_in6) };
            let bytes = sa.sin6_addr.s6_addr;
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
        self.send(&format!(":irc.test 005 {} NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#& :are supported by this server", nick));
    }
}

/// The main test handle. Created by `Eggtest::spawn()`.
pub struct Eggtest {
    pub ircd: IRCd,
}

impl Eggtest {
    /// Boot eggdrop in a background thread and wait for it to connect
    /// to the fake IRC server. Returns a handle with the IRCd socket.
    pub fn spawn() -> Eggtest {
        // Install DNS hooks
        unsafe {
            add_hook(HOOK_DNS_HOSTBYIP, rust_dns_hostbyip as *const c_void);
            add_hook(HOOK_DNS_IPBYHOST, rust_dns_ipbyhost as *const c_void);
        }

        let flags = if Path::new("LamestBot.user").exists() {
            "-n"
        } else {
            "-mn"
        };

        let args: &[CString] = Box::leak(Box::new([
            CString::new("./eggdrop").unwrap(),
            CString::new(flags).unwrap(),
            CString::new("eggdrop-basic.conf").unwrap(),
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
            },
        }
    }
}
