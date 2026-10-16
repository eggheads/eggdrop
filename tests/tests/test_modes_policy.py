"""Characterization tests for the mode *policy* layer (arbmodes3 step 0).

ARCHITECTURE.md test plan A13-A17, A22, A23: bitch, protectops/
protectfriends, revenge, bot-deop consequences, autoop/autovoice, the
desync-kick preamble, and bounce-modes. This is the surface the step-5
op+halfop policy merge rewrites, so every behaviour here must survive it.

Conventions:
- mode setters are op members (`@someop` in NAMES) so the desync-kick
  preamble doesn't fire on them;
- user records are created *before* the member is seen (negative user
  lookups are cached in the memberlist);
- "bot does NOT react to X" is asserted with a sentinel: trigger X, then
  a known-reaction Y, and require the first outbound line to be Y's.
"""

from __future__ import annotations

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
    create_test_user,
    drive_join_with_names,
    drive_registration,
    wait_onchan,
)
from support.mock_ircd import MockIrcd
from support.waiters import wait_for

SETTER = ":someop!su@sh.example.com"


def next_mode_line(mock_ircd: MockIrcd, chan: str, timeout: float = 5.0) -> str:
    return mock_ircd.drain_until(
        lambda line: line.startswith(f"MODE {chan} "), timeout=timeout
    )[-1]


def parse_mode_line(line: str) -> list[tuple[str, str]]:
    """`MODE #c -o+o someop bob` → [("-o", "someop"), ("+o", "bob")].

    Only valid for lines where every mode letter takes an argument.
    """
    words = line.split()
    sign, out = "+", []
    for c in words[2]:
        if c in "+-":
            sign = c
        else:
            out.append(sign + c)
    return list(zip(out, words[3:], strict=True))


# ---------- A13: +bitch ----------


def test_a13_bitch_deops_unauthorized_keeps_authorized(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """On a +bitch channel an op of a victim without the +o user flag is
    reverted; a victim with +o keeps the op. Enabling +bitch also sweeps
    already-opped members without +o (someop here, via the chanset-change
    recheck), so the expected deop set is {someop, alice}, grouping/timing
    of the flushes unspecified."""
    drive_registration(mock_ircd)
    create_test_user(tcl_bridge, "bob", "bob!*@*", "+o")
    chan = drive_join_with_names(
        mock_ircd, "@TestBot @someop alice bob", chanmodes_324="+nt"
    )
    wait_onchan(tcl_bridge, "bob", chan)
    tcl_bridge.eval_ok(f"channel set {chan} +bitch")

    mock_ircd.send(f"{SETTER} MODE {chan} +oo bob alice")
    deopped: set[str] = set()
    while {"someop", "alice"} - deopped:
        line = next_mode_line(mock_ircd, chan)
        words = line.split()
        modes, args = words[2], words[3:]
        assert set(modes) <= set("-o"), line
        deopped.update(args)
    assert "bob" not in deopped, deopped

    # Sentinel: no further deop (i.e. of bob) is queued behind these.
    tcl_bridge.eval_ok(f'pushmode "{chan}" +b "sentinel!*@*"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +b sentinel!*@*"


# ---------- A14: protectops / protectfriends ----------


def test_a14_protectops_reops_flagged_victim(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    create_test_user(tcl_bridge, "bob", "bob!*@*", "+o")
    chan = drive_join_with_names(
        mock_ircd, "@TestBot @someop @bob", chanmodes_324="+nt"
    )
    wait_onchan(tcl_bridge, "bob", chan)
    tcl_bridge.eval_ok(f"channel set {chan} +protectops")

    mock_ircd.send(f"{SETTER} MODE {chan} -o bob")
    line = next_mode_line(mock_ircd, chan)
    assert line == f"MODE {chan} +o bob", line


def test_a14_protectfriends_reops_friend_victim(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    create_test_user(tcl_bridge, "carol", "carol!*@*", "+f")
    chan = drive_join_with_names(
        mock_ircd, "@TestBot @someop @carol", chanmodes_324="+nt"
    )
    wait_onchan(tcl_bridge, "carol", chan)
    tcl_bridge.eval_ok(f"channel set {chan} +protectfriends")

    mock_ircd.send(f"{SETTER} MODE {chan} -o carol")
    line = next_mode_line(mock_ircd, chan)
    assert line == f"MODE {chan} +o carol", line


# ---------- A15: revenge ----------


def test_a15_revenge_punishes_deopper_of_friend(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Revenge requires +revenge AND a protection matching the victim
    (want_to_revenge: protectfriends/friend or protectops/op) — +revenge
    alone never punishes. With +revenge +protectfriends and a +f victim,
    the deopper is deopped (default revenge-mode) and the friend re-opped,
    possibly combined into one line (`-o+o someop bob`)."""
    drive_registration(mock_ircd)
    create_test_user(tcl_bridge, "bob", "bob!*@*", "+f")
    chan = drive_join_with_names(
        mock_ircd, "@TestBot @someop @bob", chanmodes_324="+nt"
    )
    wait_onchan(tcl_bridge, "bob", chan)
    tcl_bridge.eval_ok(f"channel set {chan} +revenge")
    tcl_bridge.eval_ok(f"channel set {chan} +protectfriends")

    mock_ircd.send(f"{SETTER} MODE {chan} -o bob")
    seen: list[tuple[str, str]] = []
    while ("-o", "someop") not in seen:
        seen.extend(parse_mode_line(next_mode_line(mock_ircd, chan)))
    assert ("+o", "bob") in seen, seen


# ---------- A16: bot-deop consequences ----------


def test_a16_bot_deop_fires_need_bind(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    tcl_bridge.eval_ok("set ::needlog {}")
    # type first: a leading "#" (channel) would get brace-quoted in the
    # Tcl list representation and break naive splitting.
    tcl_bridge.eval_ok(
        'proc test:needacc {chan type} {lappend ::needlog "$type@$chan"}'
    )
    tcl_bridge.eval_ok("bind need - * test:needacc")

    mock_ircd.send(f"{SETTER} MODE {chan} -o TestBot")
    wait_for(
        lambda: f"op@{chan}" in tcl_bridge.eval_ok("set ::needlog").split(),
        timeout=5.0,
        description="bind need to fire with type op",
    )


def test_a16_bot_deop_clears_sent_state(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A pending un-echoed +o suppresses re-pushes via SENTOP; the bot
    being deopped clears all SENT* bookkeeping, so after a re-op the same
    pushmode goes out again instead of being dropped."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(
        mock_ircd, "@TestBot @someop alice", chanmodes_324="+nt"
    )
    wait_onchan(tcl_bridge, "alice", chan)

    tcl_bridge.eval_ok(f'pushmode "{chan}" +o alice')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +o alice"
    # No echo for that +o: alice stays non-op with SENTOP pending.

    mock_ircd.send(f"{SETTER} MODE {chan} -o TestBot")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'botisop "{chan}"') == "0",
        timeout=5.0,
        description="bot to lose op",
    )
    mock_ircd.send(f"{SETTER} MODE {chan} +o TestBot")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'botisop "{chan}"') == "1",
        timeout=5.0,
        description="bot to regain op",
    )

    tcl_bridge.eval_ok(f'pushmode "{chan}" +o alice')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    line = next_mode_line(mock_ircd, chan)
    assert line == f"MODE {chan} +o alice", line


# ---------- A17: autoop / autovoice ----------


def test_a17_autoop_and_autovoice_on_join(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    create_test_user(tcl_bridge, "oppy", "oppy!*@*", "+o")
    create_test_user(tcl_bridge, "vicky", "vicky!*@*", "+v")
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")
    tcl_bridge.eval_ok(f"channel set {chan} aop-delay 0:0")
    tcl_bridge.eval_ok(f"channel set {chan} +autoop")
    tcl_bridge.eval_ok(f"channel set {chan} +autovoice")

    mock_ircd.send(f":oppy!ou@oh.example.com JOIN :{chan}")
    line = next_mode_line(mock_ircd, chan)
    assert line == f"MODE {chan} +o oppy", line

    mock_ircd.send(f":vicky!vu@vh.example.com JOIN :{chan}")
    line = next_mode_line(mock_ircd, chan)
    assert line == f"MODE {chan} +v vicky", line


# ---------- A22: desync/fake-mode kick ----------


def test_a22_desync_mode_by_nonop_member_kicks_and_reverses(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A MODE from a member the bot doesn't see as op/halfop (and default
    -nodesynch) gets the sender kicked and the change reversed.

    The reversal is asserted on a prefix mode (+o alice → -o alice):
    `reversing` reverts prefix modes unconditionally, while plain flag
    modes (e.g. +s) are only reverted when also chanmode-protected —
    a desynced +s is kicked for but NOT bounced (D-CHM6: intent — only a
    chanmode entry expresses a wish to enforce; pinned below).
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot bob alice", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "alice", chan)

    mock_ircd.send(f":bob!u@h.example.com MODE {chan} +o alice")
    kick = mock_ircd.drain_until(
        lambda line: line.startswith(f"KICK {chan} bob"), timeout=5.0
    )[-1]
    assert kick.startswith(f"KICK {chan} bob :"), kick
    line = next_mode_line(mock_ircd, chan)
    assert line == f"MODE {chan} -o alice", line


# ---------- A23: bounce-modes ----------


def test_a23_bounce_modes_bounces_server_not_user(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """bounce-modes 1: a server-sourced +k is reversed; a user-sourced
    mode is not (sentinel: the user +m comes first and produces no
    reaction, so the first outbound line is the -k).

    +k is used because plain flag modes (e.g. server +i) are only bounced
    when ALSO chanmode-protected — got_key has its own bounce_modes
    handling. The unprotected-flag negative is pinned separately below
    (D-CHM6).
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    tcl_bridge.eval_ok("set bounce-modes 1")

    mock_ircd.send(f"{SETTER} MODE {chan} +m")
    mock_ircd.send(f":mock.test MODE {chan} +k skey")
    line = next_mode_line(mock_ircd, chan)
    assert line == f"MODE {chan} -k skey", line


def test_a23_bounce_modes_flag_requires_chanmode_protection(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """D-CHM6 pin: under bounce-modes a flag mode is reversed only if it
    is chanmode-protected in that direction — only a chanmode entry
    expresses the user's intent to enforce, other flags are irrelevant.

    The default chanmode is +nt, so a server `-t+s` must produce exactly
    `+t` (protected) and never touch `s` (unprotected). Both reversals
    would be queued in the same gotmode pass, so a `-s` would appear in
    the same outbound line; a pushed ban then proves nothing trails.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    tcl_bridge.eval_ok("set bounce-modes 1")

    mock_ircd.send(f":mock.test MODE {chan} -t+s")
    line = next_mode_line(mock_ircd, chan)
    assert line == f"MODE {chan} +t", line

    tcl_bridge.eval_ok(f'pushmode "{chan}" +b "sentinel!*@*"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +b sentinel!*@*"
