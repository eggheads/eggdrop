use std::ffi::{CStr, CString, c_char};
use test_rs::{Eggtest, extban_flag_supported_local, extban_parse, get_extban_prefix};

/// Call extban_parse() and return (type_char, arg_string) if it's an extban, None otherwise.
fn parse_extban(mask: &str) -> Option<(char, String)> {
    let c_mask = CString::new(mask).unwrap();
    let mut typ: c_char = 0;
    let mut arg: *const c_char = std::ptr::null();
    let ret = unsafe { extban_parse(c_mask.as_ptr(), &mut typ, &mut arg) };
    if ret == 0 {
        None
    } else {
        let arg_str = if arg.is_null() {
            String::new()
        } else {
            unsafe { CStr::from_ptr(arg) }.to_string_lossy().into_owned()
        };
        Some((typ as u8 as char, arg_str))
    }
}

fn flag_supported(flag: char) -> bool {
    unsafe { extban_flag_supported_local(flag as c_char) != 0 }
}

/// Spawn eggdrop connected with given ISUPPORT, return the Eggtest handle.
fn spawn_with_extban(extban_isupport: &str) -> Eggtest {
    let mut egg = Eggtest::builder()
        .with_isupport(vec![
            &format!("NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#& WHOX {extban_isupport}"),
        ])
        .spawn();
    let nick = egg.ircd.negotiate();
    egg.ircd.send_welcome(&nick);
    egg.ircd.drain("500ms");
    egg
}

// EXTBAN ISUPPORT parsing — prefix and supported flags

#[test]
fn extban_prefix_tilde() {
    let _egg = spawn_with_extban("EXTBAN=~,aqjr ACCOUNTEXTBAN=a");

    assert!(flag_supported('a'), "a should be supported");
    assert!(flag_supported('q'), "q should be supported");
    assert!(flag_supported('j'), "j should be supported");
    assert!(flag_supported('r'), "r should be supported");
    assert!(!flag_supported('z'), "z should not be supported");
    assert!(!flag_supported('U'), "U should not be supported (not in type list)");

    assert_eq!(parse_extban("~a:someone"), Some(('a', "someone".into())));
    assert_eq!(parse_extban("a:someone"), Some(('a', "someone".into())));
    assert_eq!(parse_extban("$a:someone"), Some(('a', "someone".into())));
    assert_eq!(parse_extban("*!*@evil.com"), None);
}

#[test]
fn extban_prefix_dollar() {
    let _egg = spawn_with_extban("EXTBAN=$,aqjr ACCOUNTEXTBAN=a");

    assert!(flag_supported('a'));
    assert!(flag_supported('r'));
    assert!(!flag_supported('z'));

    assert_eq!(parse_extban("$a:someone"), Some(('a', "someone".into())));
    assert_eq!(parse_extban("a:someone"), Some(('a', "someone".into())));
    assert_eq!(parse_extban("~a:someone"), Some(('a', "someone".into())));
}

#[test]
fn extban_no_prefix() {
    let _egg = spawn_with_extban("EXTBAN=,aqjr");

    assert!(flag_supported('a'));
    assert!(flag_supported('q'));
    assert!(!flag_supported('z'));

    assert_eq!(parse_extban("a:someone"), Some(('a', "someone".into())));
    assert_eq!(parse_extban("nick!user@host"), None);
}

#[test]
fn extban_flag_supported_reflects_isupport() {
    let _egg = spawn_with_extban("EXTBAN=~,aU");

    assert!(flag_supported('a'));
    assert!(flag_supported('U'));
    assert!(!flag_supported('q'), "q not in EXTBAN types");
    assert!(!flag_supported('j'), "j not in EXTBAN types");
}

// get_extban_prefix — extracts the prefix character from ISUPPORT EXTBAN value

#[test]
fn get_extban_prefix_tilde() {
    let _egg = spawn_with_extban("EXTBAN=~,aqjr");

    let mut prefix: c_char = 0;
    unsafe { get_extban_prefix(&mut prefix) };
    assert_eq!(prefix as u8 as char, '~');
}

#[test]
fn get_extban_prefix_dollar() {
    let _egg = spawn_with_extban("EXTBAN=$,aqjr");

    let mut prefix: c_char = 0;
    unsafe { get_extban_prefix(&mut prefix) };
    assert_eq!(prefix as u8 as char, '$');
}

#[test]
fn get_extban_prefix_none() {
    let _egg = spawn_with_extban("EXTBAN=,aqjr");

    let mut prefix: c_char = 0;
    unsafe { get_extban_prefix(&mut prefix) };
    assert_eq!(prefix, 0, "no prefix should yield NUL");
}

#[test]
fn get_extban_prefix_no_extban_isupport() {
    let mut egg = Eggtest::builder()
        .with_isupport(vec![
            "NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#& WHOX",
        ])
        .spawn();
    let nick = egg.ircd.negotiate();
    egg.ircd.send_welcome(&nick);
    egg.ircd.drain("500ms");

    let mut prefix: c_char = b'X' as c_char;
    unsafe { get_extban_prefix(&mut prefix) };
    assert_eq!(prefix, 0, "no EXTBAN in ISUPPORT should clear prefix to NUL");
}
