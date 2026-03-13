use std::ffi::{c_char, c_int, c_void, CStr, CString};
use std::path::Path;

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
}

unsafe extern "C" fn rust_dns_hostbyip(addr: *mut SocknameT) {
    let family = unsafe { (*addr).family };
    let ip_str = match family {
        libc::AF_INET => {
            let sa = unsafe { &*(std::ptr::addr_of!((*addr).addr) as *const libc::sockaddr_in) };
            let ip = u32::from_be(sa.sin_addr.s_addr);
            format!("{}.{}.{}.{}", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff)
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
    println!("[rust_dns_hostbyip] reverse lookup requested for: {}", ip_str);
}

unsafe extern "C" fn rust_dns_ipbyhost(hostname: *mut c_char) {
    let name = unsafe { CStr::from_ptr(hostname) }.to_string_lossy();
    println!("[rust_dns_ipbyhost] forward lookup requested for: {}", name);
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

    let args = [
        CString::new("./eggdrop").unwrap(),
        CString::new(flags).unwrap(),
        CString::new("eggdrop-basic.conf").unwrap(),
    ];
    let mut argv: Vec<*mut c_char> = args.iter().map(|a| a.as_ptr() as *mut _).collect();
    argv.push(std::ptr::null_mut());

    unsafe {
        eggdrop_main(3, argv.as_mut_ptr());
    }
}
