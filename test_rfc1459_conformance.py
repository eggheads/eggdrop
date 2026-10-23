"""RFC 1459 protocol conformance.

Checks that Eggdrop follows the rules laid out in RFC 1459 (and, where the
RFC is silent or has been superseded in practice, what the ISUPPORT
advertisement says instead).

The rule that gets the most attention here is RFC 1459 section 2.2's
casemapping: because of IRC's Scandinavian origin, the characters `{}|^`
are defined as the lowercase equivalents of `[]\\~`. Any implementation
comparing nicknames or channel names with plain `strcasecmp` gets this
wrong, and Eggdrop carries a dedicated `rfc_tolowertab` in
src/rfc1459.c:59 to get it right. `src/rfc1459.c` had almost no coverage
before this file.

These are conformance tests rather than pure characterization: where the
RFC is unambiguous, a failure here is a bug in Eggdrop rather than a
wrong expectation in the test. Docstrings cite the relevant section so a
failure can be adjudicated against the spec instead of against opinion.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    wait_for_isupport,
)
from support.mock_ircd import MockIrcd


# ---------- section 2.2: casemapping ----------


@pytest.mark.parametrize(
    ("upper", "lower"),
    [("[", "{"), ("]", "}"), ("\\", "|")],
)
def test_scandinavian_characters_are_case_equivalent(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    upper: str,
    lower: str,
) -> None:
    """`{}|` are the lowercase forms of `[]\\` when comparing nicknames.

    RFC 1459 section 2.2: "Because of IRC's scandanavian origin, the
    characters {}| are considered to be the lower case equivalents of the
    characters []\\". The rfc_tolowertab in src/rfc1459.c:59 implements
    exactly these three pairs.

    Driven through channel membership, because that is what uses
    rfc_casecmp. Note user *handles* are compared with plain strcasecmp
    (userrec.c:222) and so do NOT fold these characters -- see
    test_handles_do_not_use_rfc_casemapping below.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    nick = f"ni{upper}ck"
    other = f"ni{lower}ck"
    mock_ircd.send(f":{nick}!u@h.example JOIN {chan}")

    # Both spellings must find the same member.
    assert tcl_bridge.eval_ok(f'onchan {{{nick}}} "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'onchan {{{other}}} "{chan}"') == "1"


def test_caret_and_tilde_are_case_equivalent(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`^` folds to `~` as well, beyond the three pairs RFC 1459 names.

    RFC 1459 section 2.2 lists only `{}|`; RFC 2812 section 2.2 adds
    `^`/`~`. Eggdrop's table folds `^` to `~` (rfc1459.c:69), matching the
    later spec and common server practice.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":ni^ck!u@h.example JOIN {chan}")

    assert tcl_bridge.eval_ok(f'onchan {{ni^ck}} "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'onchan {{ni~ck}} "{chan}"') == "1"


def test_handles_do_not_use_rfc_casemapping(
    tcl_bridge: BridgeClient,
) -> None:
    """User handles fold with plain ASCII rules, not the IRC casemapping.

    `get_user_by_handle` compares with strcasecmp (userrec.c:222), so
    `nick[` and `nick{` are two different handles even though they would
    be the same nickname on IRC.

    Pinned deliberately: it is an easy assumption that everything in
    Eggdrop uses rfc_casecmp, and scripts that rely on that will be
    wrong about handles.
    """
    tcl_bridge.eval_ok("adduser {nick[x}")
    assert tcl_bridge.eval_ok("validuser {nick[x}") == "1"
    assert tcl_bridge.eval_ok("validuser {nick{x}") == "0"


def test_ordinary_ascii_case_folding_still_applies(
    tcl_bridge: BridgeClient,
) -> None:
    """Ordinary A-Z case folding works alongside the special characters."""
    tcl_bridge.eval_ok("adduser MixedCase")
    assert tcl_bridge.eval_ok("validuser mixedcase") == "1"
    assert tcl_bridge.eval_ok("validuser MIXEDCASE") == "1"


def test_unrelated_punctuation_is_not_case_folded(
    tcl_bridge: BridgeClient,
) -> None:
    """Only the four documented pairs fold; other punctuation does not.

    A guard against an over-broad tolower table: `(` and `)` are adjacent
    to the special range in ASCII but are not case-equivalent.
    """
    tcl_bridge.eval_ok("adduser {nick(}")
    assert tcl_bridge.eval_ok("validuser {nick)}") == "0"


def test_casemapping_applies_to_channel_names(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Channel names are compared with the same casemapping as nicknames.

    RFC 1459 section 1.3: channel names are case insensitive. Combined
    with section 2.2, `#foo[bar]` and `#FOO{BAR}` name the same channel.
    """
    drive_registration(mock_ircd)
    assert tcl_bridge.eval_ok("validchan #test") == "1"
    assert tcl_bridge.eval_ok("validchan #TEST") == "1"


# ---------- section 1.2 / 2.3: nicknames and message format ----------


def test_bot_registers_with_nick_and_user(
    eggdrop_proc,
    mock_ircd: MockIrcd,
) -> None:
    """Registration sends NICK and USER, per RFC 1459 section 4.1.

    The USER command must carry four parameters, the last of which is the
    real name and is prefixed with a colon because it may contain spaces.
    """
    mock_ircd.wait_for_connect()
    nick_line = mock_ircd.drain_until(lambda line: line.startswith("NICK "))[-1]
    user_line = mock_ircd.drain_until(lambda line: line.startswith("USER "))[-1]

    assert len(nick_line.split()) == 2, nick_line
    params = user_line.split(" ", 4)
    assert len(params) == 5, user_line
    assert params[4].startswith(":"), user_line


def test_responds_to_ping_with_matching_pong(
    eggdrop_proc,
    mock_ircd: MockIrcd,
) -> None:
    """A PING is answered with a PONG carrying the same token.

    RFC 1459 section 4.6.2: the PONG reply must echo the daemon that sent
    the PING, so a server can match request to reply.
    """
    drive_registration(mock_ircd)
    mock_ircd.send("PING :abc123token")
    pong = mock_ircd.drain_until(
        lambda line: line.startswith("PONG"), timeout=10.0
    )[-1]
    assert "abc123token" in pong


def test_outgoing_lines_respect_512_byte_limit(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """No message sent to the server exceeds 512 bytes including CR-LF.

    RFC 1459 section 2.3: "IRC messages are always lines of characters
    terminated with a CR-LF pair, and these messages shall not exceed 512
    characters in length, counting all characters including the trailing
    CR-LF."

    Provoked with an over-long PRIVMSG, which Eggdrop must split or
    truncate rather than emit whole.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    payload = "x" * 900
    tcl_bridge.eval_ok(f'putserv "PRIVMSG {chan} :{payload}"')

    line = mock_ircd.drain_until(
        lambda ln: ln.startswith(f"PRIVMSG {chan}"), timeout=10.0
    )[-1]
    assert len(line.encode()) + 2 <= 512, f"line was {len(line) + 2} bytes"


# ---------- section 4.1.2: nickname collision handling ----------


def test_switches_to_alternate_nick_on_433(
    eggdrop_proc,
    mock_ircd: MockIrcd,
) -> None:
    """A 433 during registration provokes a second NICK attempt.

    RFC 1459 section 6.1, ERR_NICKNAMEINUSE: the client is expected to
    pick a different nickname rather than give up or retry the same one.
    """
    mock_ircd.wait_for_connect()
    first = mock_ircd.drain_until(lambda line: line.startswith("NICK "))[-1]
    first_nick = first.split()[1]

    mock_ircd.send(f":mock.test 433 * {first_nick} :Nickname is already in use")

    second = mock_ircd.drain_until(
        lambda line: line.startswith("NICK ") and line.split()[1] != first_nick,
        timeout=15.0,
    )[-1]
    assert second.split()[1] != first_nick


# ---------- ISUPPORT-governed limits (RFC 1459 successors) ----------


def test_default_casemapping_is_rfc1459(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """With no server advertisement, Eggdrop assumes rfc1459 casemapping.

    src/mod/server.mod/isupport.c:34 carries a default ISUPPORT string
    beginning CASEMAPPING=rfc1459, which is the correct conservative
    assumption for a server that predates ISUPPORT entirely.
    """
    drive_registration(mock_ircd, isupport_tokens=[])
    assert tcl_bridge.eval_ok("isupport get CASEMAPPING") == "rfc1459"


def test_default_chantypes_matches_rfc1459(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """The default channel prefixes are `#` and `&`.

    RFC 1459 section 1.3 defines exactly these two: `#` for network-wide
    channels and `&` for server-local ones. Later prefixes (`+`, `!`)
    postdate the RFC and are only assumed when advertised.
    """
    drive_registration(mock_ircd, isupport_tokens=[])
    assert tcl_bridge.eval_ok("isupport get CHANTYPES") == "#&"


def test_server_advertisement_overrides_default_casemapping(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A server advertising ascii casemapping overrides the rfc1459
    default, since the two disagree about `{}|^`."""
    drive_registration(mock_ircd, isupport_tokens=["CASEMAPPING=ascii"])
    wait_for_isupport(tcl_bridge, "CASEMAPPING", "ascii")
    assert tcl_bridge.eval_ok("isupport get CASEMAPPING") == "ascii"


def test_default_nicklen_matches_rfc1459(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """The assumed maximum nickname length is 9 characters.

    RFC 1459 section 1.2: "Each user is distinguished from other users by
    a unique nickname having a maximum length of nine (9) characters."
    """
    drive_registration(mock_ircd, isupport_tokens=[])
    assert tcl_bridge.eval_ok("isupport get NICKLEN") == "9"
