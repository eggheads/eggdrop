use std::time::Duration;
use test_rs::Eggtest;
use test_rs::eggdrop;

#[test]
fn eggdrop_parses_nicklen_from_isupport() {
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

    let nick_len_val = unsafe { eggdrop::nick_len };
    assert_eq!(
        nick_len_val, 16,
        "expected nick_len=16 from NICKLEN=16 in ISUPPORT, got {}",
        nick_len_val
    );
}
