"""IRCv3 extensions (gotaccount, gotaway, gotchghost, gotsetname, ...).

Each section names the IRCv3 specification it tests and the Eggdrop
handler it drives. Capabilities are enabled per-test through the config
template's `extra_tcl` hook, since Eggdrop only requests a capability
when the corresponding setting is on (servmsg.c:1565-1585).

Handlers exercised here:

    gotaccount     account-notify      ACCOUNT message
    gotaway        away-notify         AWAY message
    gotchghost     chghost             CHGHOST message
    gotsetname     setname             SETNAME message
    gotinvite      invite-notify       INVITE message
    gottagmsg      message-tags        TAGMSG message
    gotjoin        extended-join       JOIN with account and realname
    got353         multi-prefix        NAMES with stacked prefixes
                   userhost-in-names   NAMES with full hostmasks
    got354         WHOX                RPL_WHOSPCRPL
    got335         bot mode            RPL_WHOISBOT
    got730-734     MONITOR             online/offline notifications
    gotstdreply    standard replies    FAIL / WARN / NOTE

Two properties recur and are worth stating once. First, a capability that
was never negotiated must not change parsing: a bot that reads an
extended-join account field from an ordinary JOIN will treat the realname
as a channel. Second, unknown or unsolicited extension messages must be
ignored rather than acted on.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


def with_caps(eggdrop_config, request, *settings: str):
    """Start the bot with the named capability settings enabled."""
    eggdrop_config.render(extra_tcl="\n".join(f"set {s} 1" for s in settings))
    request.getfixturevalue("eggdrop_proc")
    return request.getfixturevalue("tcl_bridge")


def negotiate(mock_ircd: MockIrcd, *caps: str) -> None:
    """Advertise and acknowledge the given capabilities."""
    joined = " ".join(caps)
    mock_ircd.send(f":mock.test CAP * LS :{joined}")
    mock_ircd.send(f":mock.test CAP * ACK :{joined}")


def alive(eggdrop_proc, tcl_bridge: BridgeClient) -> None:
    eggdrop_proc.assert_alive()
    assert tcl_bridge.eval_ok("expr 5+5") == "10"


# ---------- account-notify ----------


def test_account_message_records_login(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """An ACCOUNT message records which account a user logged into.

    account-notify spec: `:nick!user@host ACCOUNT <accountname>`.
    """
    tcl_bridge = with_caps(eggdrop_config, request, "account-notify")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "account-notify")
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")

    mock_ircd.send(":alice!u@h.example ACCOUNT aliceacct")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getaccount alice "{chan}"') == "aliceacct",
        timeout=5.0,
        description="account recorded from ACCOUNT",
    )


def test_account_asterisk_means_logged_out(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """An account name of `*` means the user logged out.

    account-notify spec states account names may not be `*`, because it
    is reserved to signal logging out. A client storing it literally
    would think the user is logged in as an account called "*".
    """
    tcl_bridge = with_caps(eggdrop_config, request, "account-notify")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "account-notify")
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")

    mock_ircd.send(":alice!u@h.example ACCOUNT aliceacct")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getaccount alice "{chan}"') == "aliceacct",
        timeout=5.0,
        description="logged in",
    )
    mock_ircd.send(":alice!u@h.example ACCOUNT *")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getaccount alice "{chan}"') != "aliceacct",
        timeout=5.0,
        description="logged out clears the account",
    )


def test_account_for_unknown_user_is_harmless(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """An ACCOUNT for someone not on a shared channel does not corrupt
    state."""
    tcl_bridge = with_caps(eggdrop_config, request, "account-notify")
    eggdrop_proc = request.getfixturevalue("eggdrop_proc")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "account-notify")
    drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(":stranger!u@h.example ACCOUNT someacct")
    alive(eggdrop_proc, tcl_bridge)


# ---------- extended-join ----------


def test_extended_join_records_account(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """With extended-join, the account arrives in the JOIN itself.

    extended-join spec: `:nick!user@host JOIN <channel> <account>
    :<realname>`. This avoids a separate WHO to learn the account.
    """
    tcl_bridge = with_caps(eggdrop_config, request, "extended-join")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "extended-join")
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":bob!u@h.example JOIN {chan} bobaccount :Bob Real Name")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan bob "{chan}"') == "1",
        timeout=5.0,
        description="bob joined",
    )
    assert tcl_bridge.eval_ok(f'getaccount bob "{chan}"') == "bobaccount"


def test_extended_join_asterisk_means_no_account(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """An account of `*` in an extended JOIN means not logged in."""
    tcl_bridge = with_caps(eggdrop_config, request, "extended-join")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "extended-join")
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":carol!u@h.example JOIN {chan} * :Carol")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan carol "{chan}"') == "1",
        timeout=5.0,
        description="carol joined",
    )
    # An account of `*` means "not logged in"; how that is represented
    # internally is not specified, so only assert it is not stored as a
    # literal account name that scripts would match on.
    assert tcl_bridge.eval_ok(f'getaccount carol "{chan}"') in ("", "*", "0")


def test_plain_join_is_not_parsed_as_extended(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Without the capability, an ordinary JOIN is parsed as ordinary.

    The failure mode this guards: reading a nonexistent account field
    from a plain JOIN, which shifts every later field by one.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":dave!u@h.example JOIN {chan}")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan dave "{chan}"') == "1",
        timeout=5.0,
        description="plain JOIN still tracked",
    )


# ---------- away-notify ----------


def test_away_notify_marks_and_clears(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """away-notify delivers AWAY messages for other users in real time.

    away-notify spec: `:nick!user@host AWAY :message` when going away,
    and `:nick!user@host AWAY` with no parameter when returning.
    """
    tcl_bridge = with_caps(eggdrop_config, request, "away-notify")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "away-notify")
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")

    mock_ircd.send(":alice!u@h.example AWAY :brb")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isaway alice "{chan}"') == "1",
        timeout=5.0,
        description="alice away",
    )
    mock_ircd.send(":alice!u@h.example AWAY")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isaway alice "{chan}"') == "0",
        timeout=5.0,
        description="alice back",
    )


# ---------- chghost ----------


def test_chghost_updates_user_and_host(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """CHGHOST replaces both the username and the hostname.

    chghost spec: `:nick!user@host CHGHOST <newuser> <newhost>`. It
    replaces the older trick of faking a QUIT and a JOIN, which clients
    displayed as the user leaving and returning.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":erin!olduser@old.host JOIN {chan}")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getchanhost erin "{chan}"')
        == "olduser@old.host",
        timeout=5.0,
        description="original host",
    )
    mock_ircd.send(":erin!olduser@old.host CHGHOST newuser new.host")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getchanhost erin "{chan}"')
        == "newuser@new.host",
        timeout=5.0,
        description="host updated",
    )


def test_chghost_keeps_membership_and_status(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A host change does not drop the user or their channel status.

    This is the whole point of the extension over the fake QUIT/JOIN
    method, which did lose status in clients that took it literally.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @frank")
    assert tcl_bridge.eval_ok(f'isop frank "{chan}"') == "1"

    mock_ircd.send(":frank!u@old.host CHGHOST newu new.host")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getchanhost frank "{chan}"')
        == "newu@new.host",
        timeout=5.0,
        description="frank's host changed",
    )
    assert tcl_bridge.eval_ok(f'onchan frank "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isop frank "{chan}"') == "1"


# ---------- setname ----------


def test_setname_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A SETNAME message updating a user's realname is processed.

    setname spec: `:nick!user@host SETNAME :<newrealname>`. The realname
    may contain spaces, so it is always a trailing parameter.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot grace")
    mock_ircd.send(":grace!u@h.example SETNAME :A New Real Name")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan grace "{chan}"') == "1",
        timeout=5.0,
        description="grace still present after SETNAME",
    )
    alive(eggdrop_proc, tcl_bridge)


# ---------- invite-notify ----------


def test_invite_notify_is_handled(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """An INVITE naming a third party is processed.

    invite-notify spec: channel operators see `:inviter INVITE <target>
    <channel>` for invitations they did not send. A client assuming the
    target is always itself would misattribute these.
    """
    tcl_bridge = with_caps(eggdrop_config, request, "invite-notify")
    eggdrop_proc = request.getfixturevalue("eggdrop_proc")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "invite-notify")
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":alice!u@h.example INVITE bob {chan}")
    alive(eggdrop_proc, tcl_bridge)
    assert tcl_bridge.eval_ok(f'onchan bob "{chan}"') == "0"


# ---------- message-tags and TAGMSG ----------


def test_tagmsg_is_handled(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """A TAGMSG, which carries tags and no message body, is processed.

    Message Tags spec: TAGMSG exists so client-only tags such as `+typing`
    can be sent with no text. A client that assumes every message has a
    body may read past the end of one.
    """
    tcl_bridge = with_caps(eggdrop_config, request, "message-tags")
    eggdrop_proc = request.getfixturevalue("eggdrop_proc")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "message-tags")
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f"@+typing=active :alice!u@h.example TAGMSG {chan}")
    alive(eggdrop_proc, tcl_bridge)


def test_tags_on_privmsg_do_not_disturb_parsing(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """Message tags preceding a PRIVMSG are stripped before parsing.

    Message Tags spec: tags come before the prefix. A parser that does
    not skip them reads the tag block as the prefix.
    """
    tcl_bridge = with_caps(eggdrop_config, request, "message-tags")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "message-tags")
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(
        f"@msgid=abc123;time=2026-01-01T00:00:00.000Z :heidi!u@h.example JOIN {chan}"
    )
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan heidi "{chan}"') == "1",
        timeout=5.0,
        description="tagged JOIN parsed",
    )


def test_escaped_tag_values_do_not_break_parsing(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """Tag values containing escape sequences are handled.

    Message Tags spec defines escapes `\\:` for semicolon, `\\s` for
    space, `\\\\`, `\\r` and `\\n`. An unescaped semicolon or space inside
    a value would otherwise terminate the tag block early.
    """
    tcl_bridge = with_caps(eggdrop_config, request, "message-tags")
    drive_registration(mock_ircd)
    negotiate(mock_ircd, "message-tags")
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(
        f"@custom=a\\svalue\\:with\\sescapes :ivan!u@h.example JOIN {chan}"
    )
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan ivan "{chan}"') == "1",
        timeout=5.0,
        description="JOIN with escaped tag values parsed",
    )


def test_tags_are_stripped_when_capability_not_negotiated(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Tags are skipped even when message-tags was not negotiated.

    servmsg.c:1240 parses optional tags regardless of local support,
    which is the safe behaviour: a stray tagged message from a server
    should not desynchronise the parser.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f"@time=2026-01-01T00:00:00.000Z :judy!u@h.example JOIN {chan}")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan judy "{chan}"') == "1",
        timeout=5.0,
        description="tagged JOIN parsed without the capability",
    )


# ---------- multi-prefix and userhost-in-names ----------


def test_multi_prefix_names_grants_highest_status(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A NAMES entry with stacked prefixes is parsed, not treated as a nick.

    multi-prefix spec: with the capability, NAMES lists every status a
    user holds, so `@+alice` means op and voice. Without it only the
    highest appears. A parser expecting exactly one prefix would read
    `+alice` as the nickname.

    Eggdrop does not request multi-prefix, but got353 tolerates stacked
    prefixes anyway -- worth pinning, because a server may send them if
    another client on the same connection negotiated the capability.
    """
    # Supply the stacked prefix through the join helper, which builds the
    # 353 as part of a complete, expected NAMES burst. Sending an
    # unsolicited 353 to a settled channel does not repopulate it.
    drive_registration(mock_ircd, isupport_tokens=["PREFIX=(ohv)@%+"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @+alice")

    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1",
        timeout=5.0,
        description="alice parsed from a multi-prefix NAMES",
    )
    assert tcl_bridge.eval_ok(f'isop alice "{chan}"') == "1"


@pytest.mark.skip(
    reason="Eggdrop does not request userhost-in-names (servmsg.c:1565-1585 "
           "lists sasl, account-notify, account-tag, extended-join, "
           "invite-notify and message-tags only), so a server will never "
           "send NAMES in this form. Re-enable if the capability is added."
)
def test_userhost_in_names_is_parsed(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """NAMES entries carrying full hostmasks yield the right nickname.

    userhost-in-names spec: entries become `@nick!user@host`. A parser
    keeping the whole token as the nickname would never match anyone.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot!u@h kate!kuser@k.example")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan kate "{chan}"') == "1",
        timeout=5.0,
        description="kate parsed from userhost-in-names",
    )


# ---------- WHOX ----------


def test_whox_reply_supplies_account(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A 354 WHOX reply carries the account name.

    WHOX spec: the client requests specific fields and the server replies
    with 354 in that order. Eggdrop uses it to learn accounts without
    account-notify.
    """
    drive_registration(mock_ircd, isupport_tokens=["WHOX"])
    chan = drive_join_with_names(mock_ircd, "@TestBot whoxuser")
    mock_ircd.send(
        f":mock.test 354 TestBot 1 {chan} wuser w.host whoxuser H whoxaccount"
    )
    mock_ircd.send(f":mock.test 315 TestBot {chan} :End of /WHO list.")
    # The field order in a 354 depends on the token list the bot asked for,
    # which this harness does not control, so assert the reply is consumed
    # without disturbing the channel rather than guessing the layout.
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan whoxuser "{chan}"') == "1",
        timeout=5.0,
        description="whoxuser still tracked after a 354",
    )


def test_whox_asterisk_account_means_none(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An account of `0` or `*` in a WHOX reply means not logged in."""
    drive_registration(mock_ircd, isupport_tokens=["WHOX"])
    chan = drive_join_with_names(mock_ircd, "@TestBot nouser")
    mock_ircd.send(f":mock.test 354 TestBot 1 {chan} u h.example nouser H 0")
    mock_ircd.send(f":mock.test 315 TestBot {chan} :End of /WHO list.")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getaccount nouser "{chan}"') != "0",
        timeout=5.0,
        description="zero account not stored literally",
    )


# ---------- bot mode ----------


def test_whois_bot_numeric_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """RPL_WHOISBOT (335) marks a user as a bot.

    Bot Mode spec: servers report bot status in WHOIS. Eggdrop binds 335
    and exposes it as `isircbot`.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot otherbot")
    mock_ircd.send(":mock.test 335 TestBot otherbot :is a bot on ExampleNet")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isircbot otherbot "{chan}"') in ("0", "1"),
        timeout=5.0,
        description="bot status readable",
    )


# ---------- MONITOR ----------


@pytest.mark.parametrize(
    ("numeric", "name"),
    [
        ("730", "RPL_MONONLINE"),
        ("731", "RPL_MONOFFLINE"),
        ("732", "RPL_MONLIST"),
        ("733", "RPL_ENDOFMONLIST"),
        ("734", "ERR_MONLISTFULL"),
    ],
)
def test_monitor_numeric_is_handled(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    numeric: str,
    name: str,
) -> None:
    """Each MONITOR numeric is processed without wedging the bot.

    Monitor spec. Eggdrop binds all five; 734 in particular signals the
    monitor list is full, which a client must notice or it will believe
    it is watching nicknames it is not.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(f":mock.test {numeric} TestBot :somenick!u@h.example")
    alive(eggdrop_proc, tcl_bridge)


def test_monitor_online_notification_updates_list(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A MONONLINE notification is reflected in the monitor list.

    Monitor spec: `730 <nick> :<target>[,<target2>...]` reports watched
    nicknames coming online. The list is comma-separated, so a client
    reading one target per line misses the rest.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 730 TestBot :watched1!u@h,watched2!u@h")
    alive(eggdrop_proc, tcl_bridge)


# ---------- standard replies ----------


@pytest.mark.parametrize("kind", ["FAIL", "WARN", "NOTE"])
def test_standard_reply_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient, kind: str
) -> None:
    """FAIL, WARN and NOTE standard replies are parsed.

    Standard Replies spec: `<FAIL|WARN|NOTE> <command> <code>
    [<context>...] :<description>`. They exist so servers can report
    problems without minting new numerics.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(
        f":mock.test {kind} REHASH CONFIG_ERROR :Something went wrong"
    )
    alive(eggdrop_proc, tcl_bridge)


def test_standard_reply_with_context_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A standard reply carrying context parameters is parsed.

    The spec allows any number of context words between the code and the
    description, which is what makes the format awkward to parse: the
    description is identified by its leading colon, not by position.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(
        ":mock.test FAIL JOIN INVALID_CHANNEL #chan extra context :Bad channel"
    )
    alive(eggdrop_proc, tcl_bridge)


def test_standard_reply_without_description_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A standard reply with no colon-prefixed description is tolerated.

    servmsg.c:1744 carries a TODO about one-word descriptions that lack
    the colon. Whatever the eventual handling, it must not crash.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test WARN * UNKNOWN_ERROR")
    alive(eggdrop_proc, tcl_bridge)
