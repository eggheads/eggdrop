use std::time::Duration;
use test_rs::Eggtest;

#[test]
fn eggdrop_registers_on_connect() {
    let mut egg = Eggtest::builder()
        .with_set("nick", "Lamestbot")
        .with_set("username", "lamest")
        .spawn();

    // Eggdrop sends CAP LS 302 first (IRCv3 capability negotiation)
    egg.ircd.expect("CAP LS 302", Duration::from_secs(10));

    // Reply with empty capability list to end negotiation
    egg.ircd.send(":irc.test CAP * LS :");

    // Eggdrop sends NICK, USER, then CAP END
    let nick_line = egg.ircd.expect_prefix("NICK", Duration::from_secs(5));
    assert_eq!(nick_line[1], "Lamestbot", "unexpected nick: {}", nick_line[1]);

    let user_line = egg.ircd.expect_prefix("USER", Duration::from_secs(5));
    assert_eq!(user_line[1], "lamest", "unexpected username: {}", user_line[1]);

    egg.ircd.expect("CAP END", Duration::from_secs(5));

    // Send welcome burst
    egg.ircd.send_welcome(&nick_line[1]);

    // Eggdrop should send MODE for itself after welcome (from init-server bind)
    let mode_line = egg.ircd.expect_prefix("MODE Lamestbot", Duration::from_secs(5));
    assert_eq!(mode_line[2], "+i-ws");
}
