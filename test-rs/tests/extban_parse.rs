use std::ffi::{CStr, CString, c_char};
use test_rs::{extban_parse, is_extban_mask, extban_is_enforceable_flag};

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

// extban_parse — stateless parser, no eggdrop instance needed

#[test]
fn extban_parse_empty_and_null() {
    assert_eq!(parse_extban(""), None);
    let ret = unsafe { extban_parse(std::ptr::null(), std::ptr::null_mut(), std::ptr::null_mut()) };
    assert_eq!(ret, 0);
}

#[test]
fn extban_parse_various_type_chars() {
    for flag in ['a', 'q', 'j', 'r', 'U', 'p', 'Q', 'A', 'B', 'c', 'N', 'T'] {
        let mask = format!("~{flag}:arg");
        assert!(parse_extban(&mask).is_some(), "prefixed ~{flag}:arg should parse");
    }

    assert_eq!(parse_extban("!:arg"), None);
    assert_eq!(parse_extban(":arg"), None);
}

#[test]
fn extban_parse_arg_extraction() {
    let mask = CString::new("~a:someaccount").unwrap();
    let mut typ: c_char = 0;
    let mut arg: *const c_char = std::ptr::null();
    let ret = unsafe { extban_parse(mask.as_ptr(), &mut typ, &mut arg) };
    assert_eq!(ret, 1);
    assert_eq!(typ as u8 as char, 'a');
    let arg_str = unsafe { CStr::from_ptr(arg) }.to_str().unwrap();
    assert_eq!(arg_str, "someaccount");

    // Empty arg after colon
    assert_eq!(parse_extban("~a:"), Some(('a', "".into())));

    // Arg with special characters
    assert_eq!(parse_extban("~a:nick!user@host"), Some(('a', "nick!user@host".into())));
    assert_eq!(parse_extban("~q:*!*@*.evil.com"), Some(('q', "*!*@*.evil.com".into())));

    // Arg with colons (everything after first colon is the arg)
    assert_eq!(parse_extban("~a:foo:bar"), Some(('a', "foo:bar".into())));

    // Numeric type character
    assert_eq!(parse_extban("~1:arg"), Some(('1', "arg".into())));
    assert_eq!(parse_extban("1:arg"), Some(('1', "arg".into())));
}

#[test]
fn extban_parse_null_outparams() {
    let mask = CString::new("~a:account").unwrap();
    let ret = unsafe { extban_parse(mask.as_ptr(), std::ptr::null_mut(), std::ptr::null_mut()) };
    assert_eq!(ret, 1, "extban_parse should succeed with NULL outparams");

    // NULL type only
    let mut arg: *const c_char = std::ptr::null();
    let ret = unsafe { extban_parse(mask.as_ptr(), std::ptr::null_mut(), &mut arg) };
    assert_eq!(ret, 1);
    assert!(!arg.is_null());

    // NULL arg only
    let mut typ: c_char = 0;
    let ret = unsafe { extban_parse(mask.as_ptr(), &mut typ, std::ptr::null_mut()) };
    assert_eq!(ret, 1);
    assert_eq!(typ as u8 as char, 'a');
}

#[test]
fn extban_parse_boundary_masks() {
    // Single character — not an extban (no colon)
    assert_eq!(parse_extban("a"), None);
    assert_eq!(parse_extban("~"), None);

    // Two characters — "a:" is an extban with empty arg
    assert_eq!(parse_extban("a:"), Some(('a', "".into())));

    // Prefix + type but no colon
    assert_eq!(parse_extban("~a"), None);

    // Three characters: prefix + type + colon, empty arg
    assert_eq!(parse_extban("~a:"), Some(('a', "".into())));

    // Non-alnum type should not parse
    assert_eq!(parse_extban("~!:arg"), None);
    assert_eq!(parse_extban("~ :arg"), None);
}

#[test]
fn extban_parse_prefixed_and_unprefixed() {
    assert_eq!(parse_extban("~a:someone"), Some(('a', "someone".into())));
    assert_eq!(parse_extban("$a:someone"), Some(('a', "someone".into())));
    assert_eq!(parse_extban("a:someone"), Some(('a', "someone".into())));

    // Regular masks — not extbans
    assert_eq!(parse_extban("*!*@evil.com"), None);
    assert_eq!(parse_extban("nick!user@host"), None);
}

#[test]
fn extban_parse_word_style() {
    // Word-style extbans used by some IRCds (e.g. InspIRCd):
    //   $account:user, ~account:user
    // extban_parse should recognize these and extract the type letter
    // (first char of word) and arg (after the colon).
    assert_eq!(parse_extban("$account:user"), Some(('a', "user".into())),
        "$account:user should parse with type='a', arg='user'");
    assert_eq!(parse_extban("~account:user"), Some(('a', "user".into())),
        "~account:user should parse with type='a', arg='user'");
    // Unprefixed word-style
    assert_eq!(parse_extban("account:user"), Some(('a', "user".into())),
        "account:user should parse with type='a', arg='user'");
}

// is_extban_mask — thin wrapper around extban_parse

#[test]
fn is_extban_mask_variants() {
    let cases: &[(&str, bool)] = &[
        ("~a:account", true),
        ("$a:account", true),
        ("~q:*!*@host", true),
        ("a:account", true),
        ("q:nick!user@host", true),
        ("*!*@evil.com", false),
        ("nick!user@host", false),
        ("nick", false),
        ("*!*@*", false),
        ("", false),
        (":arg", false),
        ("!:arg", false),
        ("~:arg", false),
        ("ab:arg", true),   // parsed as prefix='a', type='b', arg="arg"
    ];

    for &(mask, expected) in cases {
        let c_mask = CString::new(mask).unwrap();
        let result = unsafe { is_extban_mask(c_mask.as_ptr()) } != 0;
        assert_eq!(result, expected, "is_extban_mask({mask:?}) expected {expected}");
    }

    assert_eq!(unsafe { is_extban_mask(std::ptr::null()) }, 0);
}

// extban_is_enforceable_flag — pure function, no state needed

#[test]
fn extban_is_enforceable_flag_checks() {
    // U is always enforceable
    assert_ne!(unsafe { extban_is_enforceable_flag(b'U' as c_char, b'a' as c_char) }, 0,
        "U should be enforceable");
    assert_ne!(unsafe { extban_is_enforceable_flag(b'U' as c_char, 0) }, 0,
        "U should be enforceable even with no account extban flag");

    // Account extban flag is enforceable when it matches
    assert_ne!(unsafe { extban_is_enforceable_flag(b'a' as c_char, b'a' as c_char) }, 0,
        "account extban flag 'a' should be enforceable when account_extban_flag='a'");
    assert_ne!(unsafe { extban_is_enforceable_flag(b'R' as c_char, b'R' as c_char) }, 0,
        "account extban flag 'R' should be enforceable when account_extban_flag='R'");

    // Non-enforceable flags
    assert_eq!(unsafe { extban_is_enforceable_flag(b'q' as c_char, b'a' as c_char) }, 0,
        "quiet 'q' should not be enforceable");
    assert_eq!(unsafe { extban_is_enforceable_flag(b'j' as c_char, b'a' as c_char) }, 0,
        "join-throttle 'j' should not be enforceable");
    assert_eq!(unsafe { extban_is_enforceable_flag(b'r' as c_char, b'a' as c_char) }, 0,
        "realname 'r' should not be enforceable");
    assert_eq!(unsafe { extban_is_enforceable_flag(b'p' as c_char, b'a' as c_char) }, 0,
        "p should not be enforceable");

    // NUL account_extban_flag — only U enforceable
    assert_eq!(unsafe { extban_is_enforceable_flag(b'a' as c_char, 0) }, 0,
        "'a' should not be enforceable when account_extban_flag is NUL");
}
