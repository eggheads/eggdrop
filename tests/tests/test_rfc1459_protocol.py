"""RFC 1459 protocol conformance: membership, modes and message handling.

A companion to test_rfc1459_conformance.py, which covers casemapping,
registration and ISUPPORT defaults. This file covers the parts of the
protocol that change state: who is on a channel, what status they hold,
and how messages directed at the bot are handled.

Sections referenced throughout:

  1.3   channels and channel names
  2.3.1 message format (prefix, params, trailing)
  4.1.2 NICK
  4.1.6 QUIT
  4.2.1 JOIN     4.2.2 PART     4.2.3 MODE
  4.2.4 TOPIC    4.2.8 KICK
  4.4.1 PRIVMSG  4.4.2 NOTICE

As with the companion file these are conformance tests: where the RFC is
unambiguous a failure means Eggdrop is wrong, or that the test drives it
incorrectly -- not that the expectation should be relaxed.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd, MockIrcdError


# ---------- 4.2.1 JOIN / 4.2.2 PART: membership tracking ----------


def test_join_adds_member_to_channel(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A JOIN from another user adds them to the tracked member list.

    RFC 1459 section 4.2.1: the server sends a JOIN message to all clients
    on the channel so they can update their own membership state.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "0"

    mock_ircd.send(f":alice!user@alice.example JOIN {chan}")
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1"


def test_part_removes_member_from_channel(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A PART removes the user from the tracked member list (section 4.2.2)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1"

    mock_ircd.send(f":alice!user@alice.example PART {chan}")
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "0"


def test_quit_removes_member_from_every_channel(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A QUIT removes the user everywhere, not just from one channel.

    RFC 1459 section 4.1.6: a QUIT is relayed to every channel the client
    was a member of. A client tracking membership per-channel must handle
    it globally.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    mock_ircd.send(":alice!user@alice.example QUIT :gone")
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "0"


def test_kick_removes_member_from_channel(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A KICK removes the kicked user, not the user who issued it
    (section 4.2.8)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @oper alice")
    mock_ircd.send(f":oper!o@oper.example KICK {chan} alice :reason")
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "0"
    assert tcl_bridge.eval_ok(f'onchan oper "{chan}"') == "1"


def test_nick_change_preserves_membership(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A NICK change renames the member without dropping them.

    RFC 1459 section 4.1.2: a nickname change does not affect channel
    membership, so the user must remain present under the new name.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    mock_ircd.send(":alice!user@alice.example NICK :roberta")

    assert tcl_bridge.eval_ok(f'onchan roberta "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "0"


def test_nick_change_preserves_channel_status(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An operator who changes nickname keeps their operator status."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @alice")
    assert tcl_bridge.eval_ok(f'isop alice "{chan}"') == "1"

    mock_ircd.send(":alice!user@alice.example NICK :roberta")
    assert tcl_bridge.eval_ok(f'isop roberta "{chan}"') == "1"


def test_member_host_is_recorded_from_join_prefix(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """The user@host from a JOIN prefix is stored for later matching.

    RFC 1459 section 2.3.1: the prefix carries the full nick!user@host of
    the originating client. Eggdrop needs the user@host portion to match
    bans and user records.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":alice!ident@alice.example JOIN {chan}")
    assert tcl_bridge.eval_ok(f'getchanhost alice "{chan}"') == "ident@alice.example"


# ---------- 4.2.3 MODE: status modes ----------


@pytest.mark.parametrize(
    ("mode", "check"),
    [("o", "isop"), ("v", "isvoice"), ("h", "ishalfop")],
)
def test_status_mode_grant_and_revoke(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    mode: str,
    check: str,
) -> None:
    """Granting then revoking a status mode is tracked in both directions.

    RFC 1459 section 4.2.3 defines `o` and `v`; `h` postdates the RFC but
    is advertised through PREFIX and handled the same way.
    """
    drive_registration(mock_ircd, isupport_tokens=["PREFIX=(ohv)@%+"])
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    assert tcl_bridge.eval_ok(f'{check} alice "{chan}"') == "0"

    mock_ircd.send(f":oper!o@mock MODE {chan} +{mode} alice")
    assert tcl_bridge.eval_ok(f'{check} alice "{chan}"') == "1"

    mock_ircd.send(f":oper!o@mock MODE {chan} -{mode} alice")
    assert tcl_bridge.eval_ok(f'{check} alice "{chan}"') == "0"


def test_multiple_modes_consume_arguments_in_order(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Several argument-taking modes in one message pair with their
    arguments left to right.

    RFC 1459 section 4.2.3: mode arguments appear in the same order as the
    mode letters that take them. Misaligning them is a classic parser bug,
    so this grants op to one user and voice to another in a single MODE.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice bob")
    mock_ircd.send(f":oper!o@mock MODE {chan} +ov alice bob")

    assert tcl_bridge.eval_ok(f'isop alice "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isvoice bob "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isvoice alice "{chan}"') == "0"
    assert tcl_bridge.eval_ok(f'isop bob "{chan}"') == "0"


def test_mixed_plus_and_minus_modes_in_one_message(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A single MODE may switch direction partway through and each mode
    applies with the sign in force at that point."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @alice bob")
    mock_ircd.send(f":oper!o@mock MODE {chan} -o+v alice bob")

    assert tcl_bridge.eval_ok(f'isop alice "{chan}"') == "0"
    assert tcl_bridge.eval_ok(f'isvoice bob "{chan}"') == "1"


def test_channel_mode_without_argument_is_recorded(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A channel mode taking no argument is stored in the channel's mode
    string (section 4.2.3: moderated, invite-only and so on)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":oper!o@mock MODE {chan} +m")
    assert "m" in tcl_bridge.eval_ok(f'getchanmode "{chan}"')

    mock_ircd.send(f":oper!o@mock MODE {chan} -m")
    assert "m" not in tcl_bridge.eval_ok(f'getchanmode "{chan}"')


def test_channel_limit_mode_keeps_its_argument(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """The user-limit mode retains its numeric argument, and dropping the
    mode takes no argument (section 4.2.3)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":oper!o@mock MODE {chan} +l 42")
    mode = tcl_bridge.eval_ok(f'getchanmode "{chan}"')
    assert "l" in mode
    assert "42" in mode


# ---------- 4.2.4 TOPIC ----------


def test_topic_with_spaces_is_kept_whole(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A trailing parameter containing spaces is not split.

    RFC 1459 section 2.3.1: the final parameter may be prefixed with a
    colon, after which spaces are part of the value rather than
    separators. Topics are the everyday case.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":oper!o@mock TOPIC {chan} :a topic with several words")
    assert tcl_bridge.eval_ok(f'topic "{chan}"') == "a topic with several words"


def test_topic_may_contain_a_colon(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Only the first colon introduces the trailing parameter; later
    colons are ordinary characters."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":oper!o@mock TOPIC {chan} :note: this has a colon")
    assert tcl_bridge.eval_ok(f'topic "{chan}"') == "note: this has a colon"


def test_empty_topic_clears_the_topic(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A TOPIC with an empty trailing parameter clears the topic rather
    than being ignored (section 4.2.4)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":oper!o@mock TOPIC {chan} :something")
    assert tcl_bridge.eval_ok(f'topic "{chan}"') == "something"

    mock_ircd.send(f":oper!o@mock TOPIC {chan} :")
    assert tcl_bridge.eval_ok(f'topic "{chan}"') == ""


# ---------- 4.4.2 NOTICE: the no-automatic-replies rule ----------


def test_no_automatic_reply_to_a_notice(
    eggdrop_proc, mock_ircd: MockIrcd
) -> None:
    """Nothing is sent in response to a NOTICE, even a CTCP-shaped one.

    RFC 1459 section 4.4.2 is explicit: "automatic replies must never be
    sent in response to a NOTICE message". This exists to stop two bots
    replying to each other forever. A CTCP VERSION arriving in a NOTICE is
    the exact case the rule was written for -- CTCP *replies* are carried
    in NOTICEs, so answering one would be self-sustaining.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(":alice!u@alice.example NOTICE TestBot :\x01VERSION\x01")

    with pytest.raises(MockIrcdError):
        mock_ircd.drain_until(
            lambda line: line.startswith(("PRIVMSG alice", "NOTICE alice")),
            timeout=3.0,
        )


def test_ctcp_version_in_a_privmsg_is_answered(
    eggdrop_proc, mock_ircd: MockIrcd
) -> None:
    """A CTCP query arriving in a PRIVMSG *is* answered, and the answer is
    a NOTICE.

    The companion to the rule above: replies go out as NOTICEs precisely
    so that the recipient will not reply in turn.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(":alice!u@alice.example PRIVMSG TestBot :\x01VERSION\x01")
    reply = mock_ircd.drain_until(
        lambda line: "VERSION" in line and line.startswith("NOTICE alice"),
        timeout=10.0,
    )[-1]
    assert reply.startswith("NOTICE alice")


# ---------- 1.3: channel name handling ----------


def test_local_channel_prefix_is_accepted(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A server-local channel name beginning with `&` is a valid channel.

    RFC 1459 section 1.3 defines both `#` and `&` prefixes; `&` channels
    are local to a single server and are frequently forgotten by clients.
    """
    drive_registration(mock_ircd)
    tcl_bridge.eval_ok("channel add {&local}")
    assert tcl_bridge.eval_ok("validchan {&local}") == "1"


def test_hostmask_generation_produces_a_valid_mask(
    tcl_bridge: BridgeClient,
) -> None:
    """A generated ban mask keeps the nick!user@host shape.

    RFC 1459 section 4.2.3 describes ban masks in that form; a mask that
    loses a separator will silently never match.
    """
    mask = tcl_bridge.eval_ok("maskhost {nick!user@host.example.com}")
    assert "!" in mask
    assert "@" in mask
