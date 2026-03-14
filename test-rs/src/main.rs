use std::ffi::{CStr, CString, c_char, c_int, c_void};
use std::path::Path;
use std::sync::{Mutex, OnceLock};
use std::thread::sleep;
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
    fn call_hostbyip(ip: *mut SocknameT, hostn: *mut c_char, ok: c_int);
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
    println!(
        "[rust_dns_hostbyip] reverse lookup requested for: {}",
        ip_str
    );
}

unsafe extern "C" fn rust_dns_ipbyhost(hostname: *mut c_char) {
    let name = unsafe { CStr::from_ptr(hostname) }.to_string_lossy();
    println!("[rust_dns_ipbyhost] forward lookup requested for: {}", name);

    if *name == *"fake.test-rs" {
        println!("[rust_dns_ipbyhost] resolving fake.test-rs to 192.0.2.1");
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

fn format_sockaddr(addr: *const libc::sockaddr) -> String {
    let family = unsafe { (*addr).sa_family } as c_int;
    match family {
        libc::AF_INET => {
            let sa = unsafe { &*(addr as *const libc::sockaddr_in) };
            let ip = u32::from_be(sa.sin_addr.s_addr);
            let port = u16::from_be(sa.sin_port);
            format!(
                "{}.{}.{}.{}:{}",
                (ip >> 24) & 0xff,
                (ip >> 16) & 0xff,
                (ip >> 8) & 0xff,
                ip & 0xff,
                port
            )
        }
        libc::AF_INET6 => {
            let sa = unsafe { &*(addr as *const libc::sockaddr_in6) };
            let bytes = sa.sin6_addr.s6_addr;
            let port = u16::from_be(sa.sin6_port);
            let segments: Vec<String> = (0..8)
                .map(|i| format!("{:x}", u16::from_be_bytes([bytes[i * 2], bytes[i * 2 + 1]])))
                .collect();
            format!("[{}]:{}", segments.join(":"), port)
        }
        _ => format!("unknown family {}", family),
    }
}

/// Our end of the fake IRC server socketpair (-1 = not connected)
static FAKE_IRCD: Mutex<c_int> = Mutex::new(-1);

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

#[unsafe(no_mangle)]
unsafe extern "C" fn connect(
    fd: c_int,
    addr: *const libc::sockaddr,
    len: libc::socklen_t,
) -> c_int {
    let dest = format_sockaddr(addr);
    println!("[rust_connect] fd={} -> {}", fd, dest);

    if is_fake_target(addr) {
        let mut pair: [c_int; 2] = [0; 2];
        if unsafe { libc::socketpair(libc::AF_UNIX, libc::SOCK_STREAM, 0, pair.as_mut_ptr()) } != 0
        {
            eprintln!("[rust_connect] socketpair() failed");
            unsafe {
                *libc::__errno_location() = libc::ECONNREFUSED;
            }
            return -1;
        }
        // pair[0] = eggdrop's end (replace fd), pair[1] = our end
        unsafe {
            libc::dup2(pair[0], fd);
            libc::close(pair[0]);
        }
        println!(
            "[rust_connect] intercepted! eggdrop fd={} <-> rust fd={}",
            fd, pair[1]
        );
        *FAKE_IRCD.lock().unwrap() = pair[1];
        return 0;
    }

    let real_connect = get_real_connect();
    unsafe { real_connect(fd, addr, len) }
}

fn main() {
    println!("Hello from Rust, about to call eggdrop_main");

    unsafe {
        add_hook(HOOK_DNS_HOSTBYIP, rust_dns_hostbyip as *const c_void);
        add_hook(HOOK_DNS_IPBYHOST, rust_dns_ipbyhost as *const c_void);
    }

    let flags = if Path::new("LamestBot.user").exists() {
        "-nt"
    } else {
        println!("No userfile found, adding -m to create one");
        "-mnt"
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
    loop {
        sleep(Duration::from_secs(1));
        println!("ping from rust");
    }
}
