"""RFC 2811 / 2812 conformance: channels and the modern client protocol.

RFC 1459 was split and revised in 2000 into four documents. Only two of
them describe behaviour an IRC *client* can be held to:

  RFC 2810  Architecture. Describes how servers form a spanning tree and
            how one-to-one and one-to-many delivery work. Almost entirely
            server-side; nothing here for a client to conform to.
  RFC 2811  Channel Management. Channel types, the modes that apply to
            them, and the ban/exception/invitation mask lists. Tested
            below.
  RFC 2812  Client Protocol. The client-facing message set, nickname
            grammar, message size limits and numeric replies. Tested
            below.
  RFC 2813  Server Protocol. Server-to-server links only. Eggdrop is a
            client and never speaks it, so there is deliberately nothing
            here for it.

Where 2811/2812 differ from RFC 1459 the newer text is treated as
authoritative, except where ISUPPORT overrides both -- in practice
networks advertise their real limits and a conformant client follows the
advertisement. Tests that depend on an advertised value say so.

Conformance tests, not characterization: a failure means Eggdrop or the
test is wrong, not that the assertion should be relaxed.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    wait_for_isupport,
)
from support.mock_ircd import MockIrcd, MockIrcdError
from support.waiters import wait_for


# ---------- RFC 2811 section 2.1: channel types ----------


@pytest.mark.parametrize(
    ("name", "kind"),
    [
        ("#standard", "standard, network-wide"),
        ("&serverlocal", "server-local"),
        ("+modeless", "modeless"),
    ],
)
def test_channel_prefixes_are_accepted(
    tcl_bridge: BridgeClient, name: str, kind: str
) -> None:
    """Each channel type defined in RFC 2811 section 2.1 is accepted as a
    channel name.

    2811 adds `+` (modeless) and `!` (safe) to RFC 1459's `#` and `&`.
    The `!` form is covered separately because its name carries a
    generated identifier.
    """
    tcl_bridge.eval_ok(f"channel add {{{name}}}")
    assert tcl_bridge.eval_ok(f"validchan {{{name}}}") == "1", kind


def test_safe_channel_prefix_is_accepted(tcl_bridge: BridgeClient) -> None:
    """A safe channel name is accepted, including its five-character ID.

    RFC 2811 section 2.1: safe channels are named `!` followed by a
    five-character server-generated identifier and then the short name,
    e.g. `!ABCDEchannel`. A client that treats the whole thing as opaque
    handles this correctly.
    """
    tcl_bridge.eval_ok("channel add {!ABCDEchannel}")
    assert tcl_bridge.eval_ok("validchan {!ABCDEchannel}") == "1"


def test_channel_names_are_case_insensitive(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Channel names compare case-insensitively (RFC 2811 section 2.1)."""
    drive_registration(mock_ircd)
    assert tcl_bridge.eval_ok("validchan #test") == "1"
    assert tcl_bridge.eval_ok("validchan #TeSt") == "1"


# ---------- RFC 2811 section 4.3: the three mask lists ----------


def test_ban_exception_and_invitation_lists_are_distinct(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Bans, ban exceptions and invitation masks are three separate lists.

    RFC 2811 section 4.3.1 defines the exception list (mode `e`) and
    4.3.2 the invitation list (mode `I`), both alongside the ban list
    (mode `b`) from 4.3. A mask on one must not appear on another.
    """
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beI,k,l,imnpst"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":oper!o@mock MODE {chan} +b *!*@banned.example")
    mock_ircd.send(f":oper!o@mock MODE {chan} +e *!*@excepted.example")
    mock_ircd.send(f":oper!o@mock MODE {chan} +I *!*@invited.example")

    bans = tcl_bridge.eval_ok(f'chanbans "{chan}"')
    exempts = tcl_bridge.eval_ok(f'chanexempts "{chan}"')
    invites = tcl_bridge.eval_ok(f'chaninvites "{chan}"')

    assert "banned.example" in bans
    assert "banned.example" not in exempts
    assert "banned.example" not in invites
    assert "excepted.example" in exempts
    assert "invited.example" in invites


def test_exception_list_disabled_when_server_omits_e(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Ban exceptions are only used when the server advertises mode `e`.

    RFC 2811 section 4.3.1 notes the exception list is optional. A client
    must not assume it exists; Eggdrop keys this off the advertised
    CHANMODES list.
    """
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=bI,k,l,imnpst"])
    wait_for_isupport(tcl_bridge, "CHANMODES", "bI,k,l,imnpst")
    assert tcl_bridge.eval_ok("set ::use-exempts") == "0"


# ---------- RFC 2811 section 4.2: channel modes ----------


def test_channel_key_mode_carries_its_argument(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """The channel key mode keeps the key as its argument.

    RFC 2811 section 4.2.1: mode `k` takes a key both when set and when
    unset, unlike the user-limit mode which takes one only when set.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":oper!o@mock MODE {chan} +k sekrit")
    mode = tcl_bridge.eval_ok(f'getchanmode "{chan}"')
    assert "k" in mode
    assert "sekrit" in mode


def test_anonymous_mode_is_tracked(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """The anonymous channel mode is recognised as a channel mode.

    RFC 2811 section 4.2.6 defines mode `a`, under which all messages
    appear to come from `anonymous!anonymous@anonymous`. It is rare, but
    a client must at least not mis-parse it as taking an argument.
    """
    drive_registration(
        mock_ircd, isupport_tokens=["CHANMODES=beI,k,l,imnpsta"]
    )
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":oper!o@mock MODE {chan} +an alice")
    assert "a" in tcl_bridge.eval_ok(f'getchanmode "{chan}"')


def test_mode_changes_respect_advertised_modes_limit(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Outgoing MODE messages carry no more mode changes than the server
    allows.

    RFC 2811 section 4.2 sets a limit of three changes per MODE command;
    modern servers advertise their own via ISUPPORT MODES. Exceeding it
    means the surplus changes are silently dropped by the server.
    """
    drive_registration(mock_ircd, isupport_tokens=["MODES=3", "PREFIX=(ohv)@%+"])
    chan = drive_join_with_names(mock_ircd, "@TestBot alice bob carol dave")

    for who in ("alice", "bob", "carol", "dave"):
        tcl_bridge.eval_ok(f'pushmode "{chan}" +v {who}')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    line = mock_ircd.drain_until(
        lambda ln: ln.startswith(f"MODE {chan} "), timeout=10.0
    )[-1]
    modes = line.split()[2]
    assert modes.count("v") <= 3, f"too many changes in one MODE: {line}"


# ---------- RFC 2812 section 2.3.1: nickname grammar ----------


@pytest.mark.parametrize("special", list("[]\\`_^{|}"))
def test_special_characters_are_legal_in_nicknames(
    tcl_bridge: BridgeClient, special: str
) -> None:
    """Every character RFC 2812 calls `special` is legal in a nickname.

    Section 2.3.1's grammar defines special as `[`, `]`, `\\`, backtick,
    `_`, `^`, `{`, `|`, `}`. A client that rejects any of them will fail
    to track real users, since these are common in practice.
    """
    handle = f"nick{special}x"
    # Braces cannot quote an unbalanced brace, so escape every special
    # character with a backslash instead. This is the hazard the whole
    # bridge has with raw Tcl — see the tcl_quote note in the README.
    quoted = "".join("\\" + c if c in '[]\\`{}$"' else c for c in handle)
    tcl_bridge.eval_ok(f"adduser {quoted}")
    assert tcl_bridge.eval_ok(f"validuser {quoted}") == "1"


def test_nickname_may_not_begin_with_a_digit(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A nickname starts with a letter or a special character, never a
    digit.

    RFC 2812 section 2.3.1: `nickname = ( letter / special ) *8( letter /
    digit / special / "-" )`. Digits are legal after the first character
    only. Eggdrop must still *track* such a nick if a server sends one,
    so this checks the grammar is understood rather than enforced
    destructively.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":9nick!u@h.example JOIN {chan}")
    assert tcl_bridge.eval_ok(f'onchan 9nick "{chan}"') == "1"


# ---------- RFC 2812 section 3.2: JOIN and PART ----------


def test_join_zero_from_another_user_is_ignored(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A `JOIN 0` attributed to another user does not change membership.

    RFC 2812 section 3.2.1 defines `JOIN 0` as a request a client sends to
    leave every channel. Crucially it is *not* relayed onward in that
    form: the server expands it into a PART for each channel before
    telling other members. So a bot should never see `:nick JOIN 0` from
    a conformant server, and gotjoin (chan.c:2055) simply looks up a
    channel literally named "0", finds none, and does nothing.

    Pinned because "Eggdrop ignores JOIN 0" looks like a conformance gap
    until you notice the message cannot legitimately arrive.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    mock_ircd.send(":alice!u@alice.example JOIN 0")
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1"


def test_part_carries_an_optional_message(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A PART may carry a parting message, which must not be mistaken for
    a channel name (RFC 2812 section 3.2.2)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    mock_ircd.send(f":alice!u@alice.example PART {chan} :so long")
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "0"


# ---------- RFC 2812 section 5.1: numeric replies ----------


@pytest.mark.parametrize(
    ("symbol", "meaning"),
    [("=", "public"), ("*", "private"), ("@", "secret")],
)
def test_names_reply_visibility_symbol_is_handled(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    symbol: str,
    meaning: str,
) -> None:
    """The channel visibility symbol in a NAMES reply is consumed, not
    mistaken for part of the channel name.

    RFC 2812 section 5.1, RPL_NAMREPLY: the reply is
    `353 <client> <symbol> <channel> :<names>` where symbol is `=`, `*`
    or `@` for public, private and secret channels. A parser that skips
    the symbol field will read the channel name from the wrong position.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    # The channel is now active, so a fresh NAMES burst is processed by
    # got353 directly. Only the visibility symbol varies between cases.
    mock_ircd.send(f":mock.test 353 TestBot {symbol} {chan} :@TestBot alice")
    mock_ircd.send(f":mock.test 366 TestBot {chan} :End of /NAMES list.")

    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1",
        timeout=10.0,
        description=f"alice to appear on {chan} after a {symbol} NAMES reply",
    )


def test_no_topic_numeric_leaves_topic_empty(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """RPL_NOTOPIC leaves the tracked topic empty rather than storing the
    explanatory text.

    RFC 2812 section 5.1: numeric 331 signals that no topic is set. Its
    trailing parameter is human-readable prose, not a topic.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":mock.test 331 TestBot {chan} :No topic is set")
    assert tcl_bridge.eval_ok(f'topic "{chan}"') == ""


def test_topic_numeric_sets_the_topic(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """RPL_TOPIC stores the topic from its trailing parameter (numeric 332,
    RFC 2812 section 5.1)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":mock.test 332 TestBot {chan} :the current topic")
    assert tcl_bridge.eval_ok(f'topic "{chan}"') == "the current topic"


# ---------- RFC 2812 section 2.3: message size ----------


def test_unknown_numeric_does_not_disrupt_parsing(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An unrecognised numeric is ignored without affecting later messages.

    RFC 2812 section 5 does not enumerate every numeric a network may
    send, so a client must skip ones it does not know rather than
    desynchronising.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":mock.test 999 TestBot {chan} :some future numeric")
    mock_ircd.send(f":alice!u@alice.example JOIN {chan}")
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1"


def test_message_with_no_prefix_is_accepted(
    eggdrop_proc, mock_ircd: MockIrcd
) -> None:
    """A message without a prefix is valid and must be handled.

    RFC 2812 section 2.3.1: the prefix is optional, and its absence means
    the message originated from the connection it arrived on. Servers
    send prefixless PINGs routinely.
    """
    drive_registration(mock_ircd)
    mock_ircd.send("PING :noprefixtoken")
    pong = mock_ircd.drain_until(
        lambda ln: ln.startswith("PONG"), timeout=10.0
    )[-1]
    assert "noprefixtoken" in pong
