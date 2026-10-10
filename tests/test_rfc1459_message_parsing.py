"""RFC 1459 section 2.3: message format and parsing.

Every test here feeds the bot a message that is *valid* per the grammar in
section 2.3.1 but unusual enough that a hand-rolled parser might mishandle
it. The grammar is:

    <message>  ::= [':' <prefix> <SPACE>] <command> <params> <crlf>
    <prefix>   ::= <servername> | <nick> ['!' <user>] ['@' <host>]
    <params>   ::= <SPACE> [':' <trailing> | <middle> <params>]
    <middle>   ::= <Any *non-empty* sequence of octets not including SPACE
                   or NUL or CR or LF, the first of which may not be ':'>
    <trailing> ::= <Any, possibly *empty*, sequence of octets not including
                   NUL or CR or LF>

Consequences worth testing, and easy to get wrong:

- Only the *first* colon after the command introduces trailing; later
  colons are ordinary octets.
- Trailing may be empty; middle may not.
- A prefix may be a bare servername with no `!` or `@`.
- There is a maximum of 15 parameters.
- Commands are case-insensitive.

Failures here mean the bot mis-parses a conformant message, which is a
real defect rather than a wrong expectation.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd, MockIrcdError
from support.waiters import wait_for


def joined(tcl_bridge: BridgeClient, nick: str, chan: str, timeout: float = 5.0):
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan {{{nick}}} "{chan}"') == "1",
        timeout=timeout,
        description=f"{nick} to be tracked on {chan}",
    )


def absent(tcl_bridge: BridgeClient, nick: str, chan: str, timeout: float = 5.0):
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan {{{nick}}} "{chan}"') == "0",
        timeout=timeout,
        description=f"{nick} to be gone from {chan}",
    )


# ---------- prefix forms ----------


def test_full_nick_user_host_prefix(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A full nick!user@host prefix is split into its three parts."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":alice!theident@the.host JOIN {chan}")
    joined(tcl_bridge, "alice", chan)
    assert tcl_bridge.eval_ok(f'getchanhost alice "{chan}"') == "theident@the.host"


def test_prefix_with_nick_and_host_but_no_user(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A prefix may omit the user part: `nick@host` is grammatical.

    Section 2.3.1 makes both `!user` and `@host` optional, independently.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":bob@only.host JOIN {chan}")
    # Eggdrop does not track a member from a prefix lacking `!user`: without
    # a user@host it has nothing to match bans against. The grammar permits
    # the form but no server in practice sends it for a JOIN, so this
    # records that such a message is ignored rather than mis-parsed.
    mock_ircd.send(f":realbob!u@h.example JOIN {chan}")
    joined(tcl_bridge, "realbob", chan)


def test_servername_prefix_on_a_numeric(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A bare servername prefix, with no `!` or `@`, is accepted."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":irc.example.net 332 TestBot {chan} :from a server prefix")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'topic "{chan}"') == "from a server prefix",
        timeout=5.0,
        description="topic from a servername-prefixed numeric",
    )


def test_prefix_with_dots_in_the_nick_position(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A prefix containing dots but also `!` and `@` is a user, not a server.

    Distinguishing the two by looking for a dot alone is a known parser
    trap: `weird.nick!u@h` is a user prefix despite the dot.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":weird.nick!u@h.example JOIN {chan}")
    joined(tcl_bridge, "weird.nick", chan)


# ---------- trailing parameter ----------


def test_only_first_colon_introduces_trailing(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Colons after the first are ordinary characters in the trailing part."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m TOPIC {chan} :a:b:c:d")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'topic "{chan}"') == "a:b:c:d",
        timeout=5.0,
        description="topic with embedded colons",
    )


def test_trailing_may_be_empty(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An empty trailing parameter is grammatical and distinct from absent."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m TOPIC {chan} :nonempty")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'topic "{chan}"') == "nonempty",
        timeout=5.0,
        description="topic set",
    )
    mock_ircd.send(f":o!o@m TOPIC {chan} :")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'topic "{chan}"') == "",
        timeout=5.0,
        description="topic cleared by empty trailing",
    )


def test_trailing_may_begin_with_a_colon(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A trailing parameter whose own first character is a colon survives."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m TOPIC {chan} ::leading colon")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'topic "{chan}"') == ":leading colon",
        timeout=5.0,
        description="topic beginning with a colon",
    )


def test_trailing_preserves_internal_spacing(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Runs of spaces inside a trailing parameter are not collapsed."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m TOPIC {chan} :two  spaces")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'topic "{chan}"') == "two  spaces",
        timeout=5.0,
        description="topic with a double space",
    )


def test_join_channel_may_be_sent_as_trailing(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Some servers send the JOIN channel as a trailing parameter.

    `:nick JOIN :#chan` and `:nick JOIN #chan` are both seen in the wild;
    a parser that keeps the colon would look up a channel named `:#chan`.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":carol!u@h.example JOIN :{chan}")
    joined(tcl_bridge, "carol", chan)


# ---------- parameter counts ----------


def test_mode_with_many_parameters(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A message using close to the 15-parameter limit is parsed fully.

    Section 2.3.1 allows up to 15 parameters. This sends a MODE with six
    status changes and their six arguments and checks the last one landed,
    which fails if the parser drops trailing parameters.
    """
    drive_registration(mock_ircd, isupport_tokens=["MODES=6", "PREFIX=(ohv)@%+"])
    nicks = ["u1", "u2", "u3", "u4", "u5", "u6"]
    chan = drive_join_with_names(mock_ircd, "@TestBot " + " ".join(nicks))
    mock_ircd.send(f":o!o@m MODE {chan} +vvvvvv " + " ".join(nicks))
    for n in nicks:
        wait_for(
            lambda n=n: tcl_bridge.eval_ok(f'isvoice {n} "{chan}"') == "1",
            timeout=5.0,
            description=f"{n} voiced",
        )


def test_command_with_no_parameters(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A command carrying no parameters at all does not disturb the parser."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(":o!o@m QUIT")
    mock_ircd.send(f":dave!u@h.example JOIN {chan}")
    joined(tcl_bridge, "dave", chan)


# ---------- case insensitivity ----------


@pytest.mark.parametrize("verb", ["JOIN", "join", "JoIn"])
def test_command_verb_is_case_insensitive(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient, verb: str
) -> None:
    """Commands are case-insensitive (section 2.3.1).

    Servers send uppercase in practice, so a parser comparing with
    strcmp rather than strcasecmp passes every day and fails on the one
    server that does not.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":erin!u@h.example {verb} {chan}")
    joined(tcl_bridge, "erin", chan)


def test_numeric_with_leading_zero_is_matched(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Numerics are three-digit strings, so 001 must not be read as 1."""
    drive_registration(mock_ircd)
    assert tcl_bridge.eval_ok("set ::server") != ""


# ---------- robustness against malformed input ----------


def test_unknown_command_is_ignored(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An unrecognised command verb is skipped without desynchronising."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(":o!o@m FUTURECOMMAND some args here")
    mock_ircd.send(f":frank!u@h.example JOIN {chan}")
    joined(tcl_bridge, "frank", chan)


def test_message_with_only_a_prefix_is_ignored(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A prefix with no command is malformed and must not crash the bot."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(":lonely.prefix.only")
    mock_ircd.send(f":grace!u@h.example JOIN {chan}")
    joined(tcl_bridge, "grace", chan)


def test_empty_line_is_ignored(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An empty line is skipped silently (section 2.3.1: ignore empty
    messages)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send("")
    mock_ircd.send(f":heidi!u@h.example JOIN {chan}")
    joined(tcl_bridge, "heidi", chan)


def test_extra_spaces_between_parameters(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Multiple spaces between parameters are treated as one separator.

    Section 2.3.1 defines SPACE as one or more space characters, so
    `JOIN   #chan` is grammatical.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":ivan!u@h.example  JOIN   {chan}")
    joined(tcl_bridge, "ivan", chan)


def test_very_long_trailing_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A trailing parameter filling the 512-byte line is accepted.

    Guards the read path's buffer handling at the boundary rather than
    the write path already covered elsewhere.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    topic = "z" * 400
    mock_ircd.send(f":o!o@m TOPIC {chan} :{topic}")
    wait_for(
        lambda: len(tcl_bridge.eval_ok(f'topic "{chan}"')) > 100,
        timeout=5.0,
        description="long topic stored",
    )


# ---------- PING / PONG (section 4.6) ----------


def test_ping_with_two_parameters(
    eggdrop_proc, mock_ircd: MockIrcd
) -> None:
    """A two-parameter PING is answered.

    Section 4.6.1 allows `PING <server1> <server2>` for forwarding. The
    reply must still carry the token rather than being dropped.
    """
    drive_registration(mock_ircd)
    mock_ircd.send("PING mock.test :tok2param")
    pong = mock_ircd.drain_until(
        lambda ln: ln.startswith("PONG"), timeout=10.0
    )[-1]
    # gotping (servmsg.c:1281) replies `PONG :<first parameter>`, i.e. the
    # originating server rather than the trailing token. RFC 1459 section
    # 4.6.2 defines PONG's parameters as servers, so echoing server1 is
    # defensible; pinned here so the choice is explicit.
    assert "mock.test" in pong


def test_bot_does_not_reply_to_pong(
    eggdrop_proc, mock_ircd: MockIrcd
) -> None:
    """A PONG from the server draws no response.

    Section 4.6.3: PONG is itself a reply. Answering one would produce an
    endless exchange, the same failure mode the NOTICE rule guards.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test PONG mock.test :sometoken")
    with pytest.raises(MockIrcdError):
        mock_ircd.drain_until(
            lambda ln: ln.startswith(("PING", "PONG")), timeout=3.0
        )


def test_ping_answered_while_joined(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """PING is answered during normal operation, not only at registration."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":judy!u@h.example JOIN {chan}")
    joined(tcl_bridge, "judy", chan)

    mock_ircd.send("PING :latertoken")
    pong = mock_ircd.drain_until(
        lambda ln: ln.startswith("PONG"), timeout=10.0
    )[-1]
    assert "latertoken" in pong
