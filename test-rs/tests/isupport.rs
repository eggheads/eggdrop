use std::time::Duration;
use test_rs::Eggtest;
use test_rs::eggdrop;

/// Read a `static mut` without creating a reference (Rust 2024 compliance).
macro_rules! read_static {
    ($sym:expr) => {
        unsafe { std::ptr::addr_of!($sym).read() }
    };
}

#[test]
fn eggdrop_parses_isupport() {
    let mut egg = Eggtest::builder()
        .with_isupport(vec![
            "ACCOUNTEXTBAN=a KNOCK SAFELIST ELIST=CMNTU MONITOR=100 CALLERID=g FNC WHOX ETRACE CHANTYPES=# EXCEPTS INVEX",
            "CHANMODES=eIbq,k,flj,CFLMPQRSTcgimnprstuz CHANLIMIT=#:250 PREFIX=(ov)@+ MAXLIST=bqeI:100 MODES=4 NETWORK=Libera.Chat STATUSMSG=@+ CASEMAPPING=rfc1459 NICKLEN=16 MAXNICKLEN=16 CHANNELLEN=50 TOPICLEN=390",
            "DEAF=D TARGMAX=NAMES:1,LIST:1,KICK:1,WHOIS:1,PRIVMSG:4,NOTICE:4,ACCEPT:,MONITOR: EXTBAN=$,agjrxz CLIENTTAGDENY=*,-typing",
        ])
        .spawn();

    let nick = egg.ircd.negotiate();
    egg.ircd.send_welcome(&nick);
    egg.ircd.drain(Duration::from_millis(500));

    assert_eq!(read_static!(eggdrop::nick_len), 16, "NICKLEN=16");
    assert_eq!(read_static!(eggdrop::use_354), 1, "WHOX sets use_354=1");
    assert_eq!(read_static!(eggdrop::modesperline), 4, "MODES=4");
    assert_eq!(read_static!(eggdrop::max_bans), 100, "MAXLIST=bqeI:100 -> max_bans=100");
    assert_eq!(read_static!(eggdrop::max_exempts), 100, "MAXLIST=bqeI:100 -> max_exempts=100");
    assert_eq!(read_static!(eggdrop::max_invites), 100, "MAXLIST=bqeI:100 -> max_invites=100");
    assert_eq!(read_static!(eggdrop::max_modes), 100, "MAXLIST=bqeI:100 -> max_modes=100");
}
