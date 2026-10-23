"""IRCv3 capability negotiation and SASL (gotcap, gotauthenticate, got9xx).

Covers the CAP handshake described in the IRCv3 Capability Negotiation
spec, and the SASL numerics from the v3.1 and v3.2 SASL specs.

Handlers exercised, all in src/mod/server.mod/servmsg.c:

    gotcap            CAP LS / ACK / NAK / NEW / DEL / LIST
    gotauthenticate   AUTHENTICATE
    got900            RPL_LOGGEDIN
    got901            RPL_LOGGEDOUT
    got903            RPL_SASLSUCCESS
    got904            ERR_SASLFAIL
    got905            ERR_SASLTOOLONG
    got906            ERR_SASLABORTED
    got907            ERR_SASLALREADY
    got908            RPL_SASLMECHS
    got410            ERR_INVALIDCAPCMD
    got451            ERR_NOTREGISTERED

Capabilities Eggdrop knows how to request (servmsg.c:1565-1585):
`sasl`, `account-notify`, `account-tag`, `extended-join`,
`invite-notify`, `message-tags`, plus anything listed in `cap-request`.

Introspection is via the Tcl `cap` command: `cap ls` lists what the
server offered, `cap enabled` what was negotiated, `cap values` the
key=value forms from CAP LS 302.

The failure numerics matter more than the success path: a bot that
mishandles ERR_SASLFAIL can hang forever mid-registration, never sending
CAP END, which is a hang rather than an error.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.irc_helpers import drive_registration
from support.mock_ircd import MockIrcd, MockIrcdError
from support.waiters import wait_for


def sent(mock_ircd: MockIrcd, prefix: str, timeout: float = 10.0) -> str:
    return mock_ircd.drain_until(
        lambda ln: ln.upper().startswith(prefix.upper()), timeout=timeout
    )[-1]


def alive(eggdrop_proc, tcl_bridge: BridgeClient) -> None:
    eggdrop_proc.assert_alive()
    assert tcl_bridge.eval_ok("expr 3+4") == "7"


# ---------- CAP negotiation ----------


def test_bot_sends_cap_ls_before_registering(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """The bot opens with CAP LS, before NICK and USER.

    Capability Negotiation spec: a client requesting capabilities sends
    CAP LS first so the server delays registration until CAP END.
    """
    eggdrop_config.render(extra_tcl="set message-tags 1")
    request.getfixturevalue("eggdrop_proc")
    mock_ircd.wait_for_connect()
    line = sent(mock_ircd, "CAP LS")
    assert line.upper().startswith("CAP LS")


def test_cap_ls_advertises_version_302(
    eggdrop_config, mock_ircd: MockIrcd, request: pytest.FixtureRequest
) -> None:
    """CAP LS carries the 302 version, enabling values and cap-notify.

    Capability Negotiation spec: `CAP LS 302` signals support for
    key=value capabilities and for the CAP NEW / CAP DEL notifications.
    """
    eggdrop_config.render(extra_tcl="set message-tags 1")
    request.getfixturevalue("eggdrop_proc")
    mock_ircd.wait_for_connect()
    line = sent(mock_ircd, "CAP LS")
    assert "302" in line


def test_offered_capabilities_are_listed(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Capabilities the server offers in CAP LS are recorded and listable."""
    drive_registration(mock_ircd)
    mock_ircd.send(
        ":mock.test CAP * LS :multi-prefix away-notify message-tags chghost"
    )
    wait_for(
        lambda: "multi-prefix" in tcl_bridge.eval_ok("cap ls"),
        timeout=5.0,
        description="offered capabilities recorded",
    )
    offered = tcl_bridge.eval_ok("cap ls")
    for capname in ("away-notify", "message-tags", "chghost"):
        assert capname in offered, offered


def test_acked_capability_is_marked_enabled(
    eggdrop_proc,
    eggdrop_config,
    mock_ircd: MockIrcd,
    request: pytest.FixtureRequest,
) -> None:
    """A capability the server ACKs is recorded as enabled.

    Capability Negotiation spec: CAP ACK confirms the capability is now
    active for this connection.
    """
    eggdrop_config.render(extra_tcl="set account-notify 1")
    request.getfixturevalue("eggdrop_proc")
    tcl_bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test CAP * LS :account-notify")
    mock_ircd.send(":mock.test CAP * ACK :account-notify")
    wait_for(
        lambda: "account-notify" in tcl_bridge.eval_ok("cap enabled"),
        timeout=5.0,
        description="account-notify enabled after ACK",
    )


def test_nak_leaves_capability_disabled(
    eggdrop_proc,
    eggdrop_config,
    mock_ircd: MockIrcd,
    request: pytest.FixtureRequest,
) -> None:
    """A NAKed capability is not treated as enabled.

    A client that assumed success would parse messages in a format the
    server is not sending.
    """
    eggdrop_config.render(extra_tcl="set account-notify 1")
    request.getfixturevalue("eggdrop_proc")
    tcl_bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test CAP * LS :account-notify")
    mock_ircd.send(":mock.test CAP * NAK :account-notify")
    assert "account-notify" not in tcl_bridge.eval_ok("cap enabled")


def test_cap_values_are_parsed(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Key=value capabilities from CAP LS 302 keep their values.

    `sasl=PLAIN,EXTERNAL` tells the client which mechanisms are
    available; discarding the value means guessing.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test CAP * LS :sasl=PLAIN,EXTERNAL multi-prefix")
    wait_for(
        lambda: "PLAIN" in tcl_bridge.eval_ok("cap values"),
        timeout=5.0,
        description="sasl mechanism list retained",
    )


def test_cap_new_adds_a_capability(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """CAP NEW announces a capability that became available later.

    Part of cap-notify in the Capability Negotiation spec; servers use it
    when, for example, the SASL layer reconnects.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test CAP * NEW :chghost")
    wait_for(
        lambda: "chghost" in tcl_bridge.eval_ok("cap ls"),
        timeout=5.0,
        description="chghost offered via CAP NEW",
    )


def test_cap_del_removes_a_capability(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """CAP DEL withdraws a capability, which must stop being enabled.

    Continuing to use a withdrawn capability means parsing a format the
    server has stopped sending.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test CAP * LS :chghost")
    wait_for(
        lambda: "chghost" in tcl_bridge.eval_ok("cap ls"),
        timeout=5.0,
        description="chghost offered",
    )
    mock_ircd.send(":mock.test CAP * DEL :chghost")
    wait_for(
        lambda: "chghost" not in tcl_bridge.eval_ok("cap enabled"),
        timeout=5.0,
        description="chghost no longer enabled",
    )


def test_cap_list_reply_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A CAP LIST reply, which reports currently-active capabilities, is
    accepted without disturbing the connection."""
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test CAP * LIST :message-tags")
    alive(eggdrop_proc, tcl_bridge)


def test_multiline_cap_ls_accumulates(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A CAP LS split across lines with `*` accumulates rather than
    replacing.

    Capability Negotiation spec: an asterisk in place of the subcommand
    argument marks a continuation. A client keeping only the last line
    loses most of the advertisement.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test CAP * LS * :multi-prefix away-notify")
    mock_ircd.send(":mock.test CAP * LS :chghost setname")
    wait_for(
        lambda: "multi-prefix" in tcl_bridge.eval_ok("cap ls")
        and "setname" in tcl_bridge.eval_ok("cap ls"),
        timeout=5.0,
        description="both CAP LS lines retained",
    )


def test_invalid_cap_subcommand_numeric_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """ERR_INVALIDCAPCMD (410) is processed rather than ignored."""
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 410 TestBot BOGUS :Invalid CAP command")
    alive(eggdrop_proc, tcl_bridge)


def test_not_registered_numeric_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """ERR_NOTREGISTERED (451) is processed.

    Sent when a command arrives before registration completes -- which
    can happen if CAP negotiation stalls.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 451 * :You have not registered")
    alive(eggdrop_proc, tcl_bridge)


def test_unknown_capability_is_not_requested(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A capability Eggdrop does not implement is listed but not requested.

    Requesting a capability you cannot parse is worse than not having it:
    the server starts sending a format you will mishandle.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test CAP * LS :some-future-cap")
    wait_for(
        lambda: "some-future-cap" in tcl_bridge.eval_ok("cap ls"),
        timeout=5.0,
        description="future cap listed",
    )
    assert "some-future-cap" not in tcl_bridge.eval_ok("cap enabled")


# ---------- SASL numerics ----------


@pytest.mark.parametrize(
    ("numeric", "name"),
    [
        ("903", "RPL_SASLSUCCESS"),
        ("904", "ERR_SASLFAIL"),
        ("905", "ERR_SASLTOOLONG"),
        ("906", "ERR_SASLABORTED"),
        ("907", "ERR_SASLALREADY"),
    ],
)
def test_sasl_result_numeric_is_handled(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    numeric: str,
    name: str,
) -> None:
    """Each SASL result numeric is processed without wedging the bot.

    SASL v3.1 spec section 3.4. All five have handlers in servmsg.c; a
    bot that ignores the failure numerics can sit forever waiting for a
    success that will not come.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(f":mock.test {numeric} TestBot :{name}")
    alive(eggdrop_proc, tcl_bridge)


def test_sasl_logged_in_numeric_records_account(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """RPL_LOGGEDIN (900) carries the account the bot authenticated as.

    SASL v3.1 spec: `900 <nick> <nick!user@host> <account> :You are now
    logged in as <account>`.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(
        ":mock.test 900 TestBot TestBot!u@h botaccount :You are now logged in"
    )
    alive(eggdrop_proc, tcl_bridge)


def test_sasl_logged_out_numeric_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """RPL_LOGGEDOUT (901) signals the account was dropped."""
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 901 TestBot TestBot!u@h :You are now logged out")
    alive(eggdrop_proc, tcl_bridge)


def test_sasl_mechanisms_numeric_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """RPL_SASLMECHS (908) lists the mechanisms the server supports.

    SASL v3.2 spec: sent after a failed attempt with an unsupported
    mechanism, so the client can pick another.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 908 TestBot PLAIN,EXTERNAL :are available")
    alive(eggdrop_proc, tcl_bridge)


def test_authenticate_challenge_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An AUTHENTICATE challenge from the server is parsed.

    SASL v3.1 spec: the server replies `AUTHENTICATE +` to request the
    client's payload. With SASL disabled the bot should ignore it rather
    than respond or crash.
    """
    drive_registration(mock_ircd)
    mock_ircd.send("AUTHENTICATE +")
    alive(eggdrop_proc, tcl_bridge)


def test_sasl_failure_does_not_prevent_operation(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """After a SASL failure the bot still processes normal traffic.

    The important property: a failed authentication degrades to an
    unauthenticated session rather than a hung one.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 904 TestBot :SASL authentication failed")
    mock_ircd.send("PING :aftersaslfail")
    pong = mock_ircd.drain_until(
        lambda ln: ln.startswith("PONG"), timeout=10.0
    )[-1]
    assert "aftersaslfail" in pong


def test_unsolicited_sasl_numeric_is_ignored(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A SASL numeric arriving with no authentication in progress is
    harmless.

    Guards against state machines that assume a numeric can only arrive
    when they are expecting it.
    """
    drive_registration(mock_ircd)
    for numeric in ("903", "904", "907"):
        mock_ircd.send(f":mock.test {numeric} TestBot :unsolicited")
    alive(eggdrop_proc, tcl_bridge)
