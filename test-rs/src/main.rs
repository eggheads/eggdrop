use std::ffi::CString;
use std::path::Path;

unsafe extern "C" {
    fn eggdrop_main(argc: std::ffi::c_int, argv: *mut *mut std::ffi::c_char) -> std::ffi::c_int;
}

fn main() {
    println!("Hello from Rust, about to call eggdrop_main");

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
    let mut argv: Vec<*mut std::ffi::c_char> = args.iter().map(|a| a.as_ptr() as *mut _).collect();
    argv.push(std::ptr::null_mut());

    unsafe {
        eggdrop_main(3, argv.as_mut_ptr());
    }
}
