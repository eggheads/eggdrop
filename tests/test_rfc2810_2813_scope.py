"""RFC 2810 (Architecture) and RFC 2813 (Server Protocol): scope.

Neither document imposes much on a client, and this file is deliberately
short. Manufacturing twenty tests per RFC here would mean inventing
obligations the specifications do not create.

**RFC 2810 — Architecture.** Describes the spanning-tree topology, the
one-to-one and one-to-many delivery models, and the resulting problems
(scalability, netsplits, nickname collisions). Almost all of it
constrains servers. The one client-visible consequence worth testing is
that a client must survive the network reorganising underneath it: a
netsplit removes users it never saw leave, and a netjoin returns them.
Eggdrop tracks split members specifically for this (`onchansplit`,
`wasop`), which makes it testable.

**RFC 2813 — Server Protocol.** The link protocol between servers:
PASS/SERVER handshakes, NJOIN, SQUIT, the server-only message set.
Eggdrop connects as a client and never speaks any of it. The only
meaningful property is negative: server-protocol messages arriving on a
client connection must be ignored rather than acted upon. A client that
honoured NJOIN or SQUIT from an untrusted source would be trivially
confusable, so these tests are worth having even though they are few.

That is the honest extent of it. The substantive conformance work lives
in the RFC 1459, 2811 and 2812 files.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


# ---------- RFC 2810 section 5.2: netsplits ----------


def test_netsplit_quit_removes_members(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A netsplit arrives as QUIT messages and removes the affected users.

    RFC 2810 section 5.2 describes network fragmentation. Servers signal
    it by quitting every user behind the lost link, conventionally with a
    two-servername quit message.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice bob")
    mock_ircd.send(":alice!u@h.example QUIT :hub.example leaf.example")
    # A split user is marked split rather than deleted -- Eggdrop keeps the
    # record so ops can be restored when the network heals. Users on the
    # surviving side are untouched.
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchansplit alice "{chan}"') == "1",
        timeout=5.0,
        description="alice marked as split",
    )
    assert tcl_bridge.eval_ok(f'onchan bob "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'onchansplit bob "{chan}"') == "0"


def test_split_member_is_remembered_as_split(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A user lost to a netsplit is remembered as split, not merely gone.

    This is what lets a bot restore op status when the network heals
    instead of treating the returning user as a stranger.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @alice")
    mock_ircd.send(":alice!u@h.example QUIT :hub.example leaf.example")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchansplit alice "{chan}"') == "1",
        timeout=5.0,
        description="alice recorded as split",
    )


def test_ordinary_quit_is_not_treated_as_a_split(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A normal QUIT message is not mistaken for a netsplit.

    The distinction is made on the shape of the quit message -- two
    servernames and nothing else. A client that guessed wrong would hold
    ops open for users who left deliberately.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @alice")
    mock_ircd.send(":alice!u@h.example QUIT :going to bed")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "0",
        timeout=5.0,
        description="alice left",
    )
    assert tcl_bridge.eval_ok(f'onchansplit alice "{chan}"') == "0"


def test_returning_split_member_is_recognised(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A user who rejoins after a split is tracked again on rejoin.

    RFC 2810 section 5.2 notes the network reconnects and state must be
    reconciled; from the client side that means the returning user is a
    member once more.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @alice")
    mock_ircd.send(":alice!u@h.example QUIT :hub.example leaf.example")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchansplit alice "{chan}"') == "1",
        timeout=5.0,
        description="alice split",
    )

    mock_ircd.send(f":alice!u@h.example JOIN {chan}")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1",
        timeout=5.0,
        description="alice returned",
    )


# ---------- RFC 2813: server-only messages must not be honoured ----------


@pytest.mark.parametrize(
    ("message", "section"),
    [
        ("SERVER evil.example 1 :Evil Server", "4.1.2"),
        ("NJOIN #test :@alice,bob", "4.2.2"),
        ("SQUIT mock.test :bye", "4.1.7"),
        ("SVINFO 3 3 0 :1700000000", "non-standard, TS protocol"),
    ],
)
def test_server_protocol_message_is_ignored(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    message: str,
    section: str,
) -> None:
    """A server-to-server message arriving on a client link is ignored.

    RFC 2813 defines these for links between servers. Eggdrop is a
    client: it must neither act on them nor break parsing the messages
    that follow. Acting on NJOIN in particular would let anything that
    can send the bot a line rewrite its member list.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":evil.example {message}")

    # The bot ignored it and is still parsing normally.
    mock_ircd.send(f":later!u@h.example JOIN {chan}")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan later "{chan}"') == "1",
        timeout=5.0,
        description=f"parsing continues after a {section} message",
    )
    eggdrop_proc.assert_alive()


def test_njoin_does_not_add_members(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """NJOIN must not populate the member list.

    RFC 2813 section 4.2.2: NJOIN is how one server tells another about
    existing channel members during a link burst. Honouring it from a
    client connection would let an attacker inject phantom members --
    and phantom operators.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":evil.example NJOIN {chan} :@phantom,ghost")

    assert tcl_bridge.eval_ok(f'onchan phantom "{chan}"') == "0"
    assert tcl_bridge.eval_ok(f'onchan ghost "{chan}"') == "0"
    assert tcl_bridge.eval_ok(f'isop phantom "{chan}"') == "0"
