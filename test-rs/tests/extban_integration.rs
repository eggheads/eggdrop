use test_rs::{ChannelMember, Eggtest};

/// Extract ban masks from the Tcl `chanbans` result.
fn chanbans_masks(egg: &mut Eggtest, chan: &str) -> Vec<String> {
    let raw = egg.tcl(&format!("chanbans {chan}"));
    if raw.is_empty() {
        return Vec::new();
    }
    let count: usize = egg.tcl(&format!("llength [chanbans {chan}]")).parse().unwrap_or(0);
    let mut masks = Vec::new();
    for i in 0..count {
        let mask = egg.tcl(&format!("lindex [lindex [chanbans {chan}] {i}] 0"));
        masks.push(mask);
    }
    masks
}

#[test]
fn channel_bans_include_extbans() {
    let mut egg = Eggtest::builder()
        .with_isupport(vec![
            "NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#& WHOX EXTBAN=~,aqU ACCOUNTEXTBAN=a",
        ])
        .with_channels(vec!["#test"])
        .spawn();
    let nick = egg.ircd.negotiate();
    egg.ircd.send_welcome(&nick);
    egg.ircd.drain("500ms");

    let members = [
        ChannelMember { nick: "opnick", user: "op", host: "op.host", flags: "H@", account: "opacct" },
    ];
    let bans = [
        "*!*@evil.com opnick 1700000000",
        "~a:badaccount opnick 1700000000",
        "~q:*!*@quiet.com opnick 1700000000",
    ];
    egg.ircd.join_chan(&nick, "lamest", "test.host", "#test", &members, &bans);
    egg.ircd.drain("500ms");

    let masks = chanbans_masks(&mut egg, "#test");
    assert!(masks.contains(&"*!*@evil.com".to_string()), "regular ban missing from chanbans: {masks:?}");
    assert!(masks.contains(&"~a:badaccount".to_string()), "extban ~a:badaccount missing from chanbans: {masks:?}");
    assert!(masks.contains(&"~q:*!*@quiet.com".to_string()), "extban ~q:*!*@quiet.com missing from chanbans: {masks:?}");
}

#[test]
fn non_enforceable_extban_is_sticky() {
    let mut egg = Eggtest::builder()
        .with_isupport(vec![
            "NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#& WHOX EXTBAN=~,aqU ACCOUNTEXTBAN=a",
        ])
        .with_channels(vec!["#test"])
        .spawn();
    let nick = egg.ircd.negotiate();
    egg.ircd.send_welcome(&nick);
    egg.ircd.drain("500ms");

    let members = [
        ChannelMember { nick: "opnick", user: "op", host: "op.host", flags: "H@", account: "opacct" },
    ];
    egg.ircd.join_chan(&nick, "lamest", "test.host", "#test", &members, &[]);
    egg.ircd.drain("500ms");

    // Enforceable extban (account ban ~a:) — should NOT be sticky
    egg.tcl("newchanban {#test} {~a:badguy} LamestBot {account ban}");
    let sticky_a = egg.tcl("isbansticky {~a:badguy} {#test}");
    assert_eq!(sticky_a, "0", "account extban ~a: should NOT be sticky (enforceable)");

    // Enforceable extban (U = registered nicks) — should NOT be sticky
    egg.tcl("newchanban {#test} {~U:*!*@evil.com} LamestBot {unregistered ban}");
    let sticky_u = egg.tcl("isbansticky {~U:*!*@evil.com} {#test}");
    assert_eq!(sticky_u, "0", "U extban should NOT be sticky (enforceable)");

    // Non-enforceable extban (quiet ~q:) — should be auto-sticky
    egg.tcl("newchanban {#test} {~q:*!*@quiet.com} LamestBot {quiet ban}");
    let sticky_q = egg.tcl("isbansticky {~q:*!*@quiet.com} {#test}");
    assert_eq!(sticky_q, "1", "quiet extban ~q: should be auto-sticky (not enforceable)");
}

#[test]
fn non_enforceable_extban_is_sticky_word_style_accountextban() {
    // ACCOUNTEXTBAN=a,account — comma-separated format used by some IRCds
    let mut egg = Eggtest::builder()
        .with_isupport(vec![
            "NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#& WHOX EXTBAN=$,aqU ACCOUNTEXTBAN=a,account",
        ])
        .with_channels(vec!["#test"])
        .spawn();
    let nick = egg.ircd.negotiate();
    egg.ircd.send_welcome(&nick);
    egg.ircd.drain("500ms");

    let members = [
        ChannelMember { nick: "opnick", user: "op", host: "op.host", flags: "H@", account: "opacct" },
    ];
    egg.ircd.join_chan(&nick, "lamest", "test.host", "#test", &members, &[]);
    egg.ircd.drain("500ms");

    // Single-letter account extban — should NOT be sticky (enforceable via 'a' flag)
    egg.tcl("newchanban {#test} {$a:badguy} LamestBot {account ban}");
    let sticky_a = egg.tcl("isbansticky {$a:badguy} {#test}");
    assert_eq!(sticky_a, "0", "account extban $a: should NOT be sticky with ACCOUNTEXTBAN=a,account");

    // Word-style account extban — should also NOT be sticky (enforceable)
    egg.tcl("newchanban {#test} {$account:badguy} LamestBot {word-style account ban}");
    let sticky_word = egg.tcl("isbansticky {$account:badguy} {#test}");
    assert_eq!(sticky_word, "0", "$account:badguy should NOT be sticky with ACCOUNTEXTBAN=a,account");

    // Non-enforceable extban — should be auto-sticky
    egg.tcl("newchanban {#test} {$q:*!*@quiet.com} LamestBot {quiet ban}");
    let sticky_q = egg.tcl("isbansticky {$q:*!*@quiet.com} {#test}");
    assert_eq!(sticky_q, "1", "quiet extban $q: should be auto-sticky");
}

#[test]
fn extban_enforcement_on_join() {
    let mut egg = Eggtest::builder()
        .with_isupport(vec![
            "NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#& WHOX EXTBAN=~,aqU ACCOUNTEXTBAN=a",
        ])
        .with_channels(vec!["#test"])
        .spawn();

    // Custom CAP negotiation: offer extended-join so eggdrop gets account on JOIN
    egg.ircd.expect_prefix("CAP LS", "10s");
    egg.ircd.send(":irc.test CAP * LS :extended-join account-notify");
    let nick_line = egg.ircd.expect_prefix("NICK", "5s");
    egg.ircd.expect_prefix("USER", "5s");
    let cap_req = egg.ircd.expect_prefix("CAP REQ", "5s");
    let caps = cap_req.get(2).unwrap_or("");
    egg.ircd.send(&format!(":irc.test CAP * ACK :{caps}"));
    egg.ircd.expect("CAP END", "5s");
    let nick = nick_line[1].clone();

    egg.ircd.send_welcome(&nick);
    egg.ircd.drain("500ms");

    let members = [
        ChannelMember { nick: "opnick", user: "op", host: "op.host", flags: "H@", account: "opacct" },
    ];
    egg.ircd.join_chan(&nick, "lamest", "test.host", "#test", &members, &[]);
    egg.ircd.drain("500ms");

    // Op the bot
    egg.ircd.send(&format!(":opnick!op@op.host MODE #test +o {nick}"));
    egg.ircd.drain("1s");

    // Enable ban enforcement and add extban for account "badguy"
    egg.tcl("channel set {#test} +enforcebans");
    egg.tcl("newchanban {#test} {~a:badguy} LamestBot {extban test}");
    egg.ircd.drain("1s");

    // User with account "badguy" joins (extended-join format includes account)
    egg.ircd.send(":evil!user@evil.host JOIN #test badguy :Evil User");

    // Watch for KICK — respond to any WHO queries along the way
    loop {
        let line = egg.ircd.expect_prefix("", "10s");
        eprintln!("[test] eggdrop sent: {line}");
        if line.get(0).is_some_and(|cmd| cmd == "WHO") {
            egg.ircd.send(&format!(":irc.test 354 {nick} 222 #test user evil.host evil H badguy"));
            egg.ircd.send(&format!(":irc.test 315 {nick} #test :End of /WHO list."));
        } else if line.get(0).is_some_and(|cmd| cmd == "KICK") {
            assert!(line.get(2).is_some_and(|who| who == "evil"), "expected KICK for evil, got: {line}");
            return;
        }
    }
}

#[test]
fn account_change_triggers_extban_kick() {
    let mut egg = Eggtest::builder()
        .with_isupport(vec![
            "NETWORK=TestNet CASEMAPPING=rfc1459 CHANTYPES=#& WHOX EXTBAN=~,aqU ACCOUNTEXTBAN=a",
        ])
        .with_channels(vec!["#test"])
        .spawn();

    // Negotiate with extended-join and account-notify CAPs
    egg.ircd.expect_prefix("CAP LS", "10s");
    egg.ircd.send(":irc.test CAP * LS :extended-join account-notify");
    let nick_line = egg.ircd.expect_prefix("NICK", "5s");
    egg.ircd.expect_prefix("USER", "5s");
    let cap_req = egg.ircd.expect_prefix("CAP REQ", "5s");
    let caps = cap_req.get(2).unwrap_or("");
    egg.ircd.send(&format!(":irc.test CAP * ACK :{caps}"));
    egg.ircd.expect("CAP END", "5s");
    let nick = nick_line[1].clone();

    egg.ircd.send_welcome(&nick);
    egg.ircd.drain("500ms");

    // Bot joins channel with an innocent user (no account yet)
    let members = [
        ChannelMember { nick: "opnick", user: "op", host: "op.host", flags: "H@", account: "opacct" },
        ChannelMember { nick: "innocent", user: "user", host: "innocent.host", flags: "H", account: "*" },
    ];
    egg.ircd.join_chan(&nick, "lamest", "test.host", "#test", &members, &[]);
    egg.ircd.drain("500ms");

    // Op the bot
    egg.ircd.send(&format!(":opnick!op@op.host MODE #test +o {nick}"));
    egg.ircd.drain("1s");

    // Enable ban enforcement and add extban for account "targetaccount"
    egg.tcl("channel set {#test} +enforcebans");
    egg.tcl("newchanban {#test} {~a:targetaccount} LamestBot {extban test}");
    egg.ircd.drain("1s");

    // User logs into account "targetaccount" via ACCOUNT message
    egg.ircd.send(":innocent!user@innocent.host ACCOUNT targetaccount");

    // Expect eggdrop to kick the user
    loop {
        let line = egg.ircd.expect_prefix("", "10s");
        eprintln!("[test] eggdrop sent: {line}");
        if line.get(0).is_some_and(|cmd| cmd == "WHO") {
            egg.ircd.send(&format!(":irc.test 354 {nick} 222 #test user innocent.host innocent H targetaccount"));
            egg.ircd.send(&format!(":irc.test 315 {nick} #test :End of /WHO list."));
        } else if line.get(0).is_some_and(|cmd| cmd == "KICK") {
            assert!(line.get(2).is_some_and(|who| who == "innocent"), "expected KICK for innocent, got: {line}");
            return;
        }
    }
}
