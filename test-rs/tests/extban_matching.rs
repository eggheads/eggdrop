use std::ffi::CString;
use test_rs::{Eggtest, eggdrop, banmask_matches_member};

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

fn make_member(nick: &str, userhost: &str, account: &str) -> eggdrop::memberlist {
    let mut m = eggdrop::memberlist::default();
    let nick_bytes = nick.as_bytes();
    let uh_bytes = userhost.as_bytes();
    let acct_bytes = account.as_bytes();
    m.nick[..nick_bytes.len()].copy_from_slice(unsafe {
        std::slice::from_raw_parts(nick_bytes.as_ptr() as *const i8, nick_bytes.len())
    });
    m.userhost[..uh_bytes.len()].copy_from_slice(unsafe {
        std::slice::from_raw_parts(uh_bytes.as_ptr() as *const i8, uh_bytes.len())
    });
    m.account[..acct_bytes.len()].copy_from_slice(unsafe {
        std::slice::from_raw_parts(acct_bytes.as_ptr() as *const i8, acct_bytes.len())
    });
    m
}

#[test]
fn banmask_matches_member_regular_ban() {
    let _egg = spawn_with_extban("EXTBAN=~,aqU ACCOUNTEXTBAN=a");

    let mut m = make_member("evil", "user@evil.host", "*");
    let banmask = CString::new("*!*@evil.host").unwrap();
    let user = CString::new("evil!user@evil.host").unwrap();

    let result = unsafe { banmask_matches_member(banmask.as_ptr(), user.as_ptr(), &mut m) };
    assert_ne!(result, 0, "regular ban *!*@evil.host should match evil!user@evil.host");

    let banmask2 = CString::new("*!*@good.host").unwrap();
    let result2 = unsafe { banmask_matches_member(banmask2.as_ptr(), user.as_ptr(), &mut m) };
    assert_eq!(result2, 0, "regular ban *!*@good.host should not match evil!user@evil.host");
}

#[test]
fn banmask_matches_member_account_extban() {
    let _egg = spawn_with_extban("EXTBAN=~,aqU ACCOUNTEXTBAN=a");

    let mut m = make_member("evil", "user@evil.host", "badguy");
    let user = CString::new("evil!user@evil.host").unwrap();

    // Matching account extban
    let ban = CString::new("~a:badguy").unwrap();
    let result = unsafe { banmask_matches_member(ban.as_ptr(), user.as_ptr(), &mut m) };
    assert_ne!(result, 0, "~a:badguy should match member with account 'badguy'");

    // Non-matching account extban
    let ban2 = CString::new("~a:goodguy").unwrap();
    let result2 = unsafe { banmask_matches_member(ban2.as_ptr(), user.as_ptr(), &mut m) };
    assert_eq!(result2, 0, "~a:goodguy should not match member with account 'badguy'");

    // Unprefixed account extban
    let ban3 = CString::new("a:badguy").unwrap();
    let result3 = unsafe { banmask_matches_member(ban3.as_ptr(), user.as_ptr(), &mut m) };
    assert_ne!(result3, 0, "a:badguy (unprefixed) should match member with account 'badguy'");
}

#[test]
fn banmask_matches_member_no_account() {
    let _egg = spawn_with_extban("EXTBAN=~,aqU ACCOUNTEXTBAN=a");

    let mut m_noacct = make_member("user1", "user@host", "");
    let user = CString::new("user1!user@host").unwrap();

    let ban = CString::new("~a:someaccount").unwrap();
    let result = unsafe { banmask_matches_member(ban.as_ptr(), user.as_ptr(), &mut m_noacct) };
    assert_eq!(result, 0, "account extban should not match member with no account");
}

#[test]
fn banmask_matches_member_case_insensitive_account() {
    let _egg = spawn_with_extban("EXTBAN=~,aqU ACCOUNTEXTBAN=a");

    let mut m = make_member("nick", "user@host", "BadGuy");
    let user = CString::new("nick!user@host").unwrap();

    let ban = CString::new("~a:badguy").unwrap();
    let result = unsafe { banmask_matches_member(ban.as_ptr(), user.as_ptr(), &mut m) };
    assert_ne!(result, 0, "account extban should match case-insensitively");

    let ban2 = CString::new("~a:BADGUY").unwrap();
    let result2 = unsafe { banmask_matches_member(ban2.as_ptr(), user.as_ptr(), &mut m) };
    assert_ne!(result2, 0, "account extban should match case-insensitively (uppercase)");
}

#[test]
fn banmask_matches_member_word_style_account_extban() {
    // ACCOUNTEXTBAN=a,account — comma-separated format (flag letter, human name)
    let _egg = spawn_with_extban("EXTBAN=$,aqU ACCOUNTEXTBAN=a,account");

    let mut m = make_member("evil", "user@evil.host", "badguy");
    let user = CString::new("evil!user@evil.host").unwrap();

    // Single-letter style should still work
    let ban = CString::new("$a:badguy").unwrap();
    let result = unsafe { banmask_matches_member(ban.as_ptr(), user.as_ptr(), &mut m) };
    assert_ne!(result, 0, "$a:badguy should match with ACCOUNTEXTBAN=a,account");

    // Word-style: $account:badguy
    let ban2 = CString::new("$account:badguy").unwrap();
    let result2 = unsafe { banmask_matches_member(ban2.as_ptr(), user.as_ptr(), &mut m) };
    assert_ne!(result2, 0, "$account:badguy should match with ACCOUNTEXTBAN=a,account");
}
