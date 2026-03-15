use test_rs::Eggtest;

#[test]
fn eggdrop_responds_to_ping() {
    let mut egg = Eggtest::spawn();

    let nick = egg.ircd.negotiate();
    egg.ircd.send_welcome(&nick);
    egg.ircd.drain("500ms");

    egg.ircd.send("PING :12345");
    egg.ircd.expect("PONG :12345", "5s");
}
