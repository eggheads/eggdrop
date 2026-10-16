"""Characterization tests for chanmode/ban enforcement (arbmodes3 step 0).

ARCHITECTURE.md test plan A10-A12, A18-A21, A24-A26, A28: the chanmode
protection machinery (set_mode_protect / recheck_channel_modes / the
gotmode enforcement branches), ban/exempt/invite reactions, list tracking,
chanmode persistence, partyline commands, and bind re-entrancy.

Live flag enforcement only reacts to letters present in the chanmode
protection string (`+nt-i` reverts +i and -t; an unprotected +s is
ignored) — that is the current contract being pinned.
"""

from __future__ import annotations

import re

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
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


# ---------- A10: chanmode flag enforcement ----------


def test_a10_chanmode_flag_enforce_live(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """chanmode `+nt-i`: inbound +i is reverted, inbound -t is re-added."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    tcl_bridge.eval_ok(f'channel set {chan} chanmode "+nt-i"')

    mock_ircd.send(f"{SETTER} MODE {chan} +i")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} -i"

    mock_ircd.send(f"{SETTER} MODE {chan} -t")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +t"


def test_a10_chanmode_flag_enforce_on_join(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Joining as op with 324 showing only +n: the missing +t from the
    default chanmode +nt is pushed by the end-of-WHO recheck."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+n")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +t"


# ---------- A11: key enforcement + JOIN key ----------


def test_a11_join_key_and_key_readd(
    eggdrop_config,  # EggdropConfig lives in conftest
    mock_ircd: MockIrcd,
    request: pytest.FixtureRequest,
) -> None:
    """chanmode `+ntk wantkey`: the bot JOINs with the desired key; a -k by
    another op is countered by re-adding the desired key; a stranger
    setting a different key gets it replaced."""
    eggdrop_config.render(
        channels=[{"name": "#test", "chanmode": "+ntk wantkey"}]
    )
    request.getfixturevalue("eggdrop_proc")
    tcl_bridge = request.getfixturevalue("tcl_bridge")

    drive_registration(mock_ircd)
    join_line = mock_ircd.drain_until(
        lambda line: line.startswith("JOIN "), timeout=10.0
    )[-1]
    assert join_line == "JOIN #test wantkey", join_line
    chan = drive_join_with_names(
        mock_ircd,
        "@TestBot @someop",
        chanmodes_324="+ntk wantkey",
        join_line=join_line,
    )
    wait_onchan(tcl_bridge, "someop", chan)

    mock_ircd.send(f"{SETTER} MODE {chan} -k wantkey")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +k wantkey"

    mock_ircd.send(f"{SETTER} MODE {chan} +k otherkey")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +k wantkey"


def test_a11_chanmode_minus_k_bounces_keys(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """chanmode `+nt-k`: any key set by a non-master is removed."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    tcl_bridge.eval_ok(f'channel set {chan} chanmode "+nt-k"')

    mock_ircd.send(f"{SETTER} MODE {chan} +k foo")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} -k foo"


# ---------- A12: limit enforcement ----------


def test_a12_limit_enforcement(
    eggdrop_config,
    mock_ircd: MockIrcd,
    request: pytest.FixtureRequest,
) -> None:
    """chanmode `+ntl 50`: removing the limit re-adds 50; setting a
    different limit is corrected back to 50."""
    eggdrop_config.render(channels=[{"name": "#test", "chanmode": "+ntl 50"}])
    request.getfixturevalue("eggdrop_proc")
    tcl_bridge = request.getfixturevalue("tcl_bridge")

    drive_registration(mock_ircd)
    chan = drive_join_with_names(
        mock_ircd, "@TestBot @someop", chanmodes_324="+ntl 50"
    )
    wait_onchan(tcl_bridge, "someop", chan)

    mock_ircd.send(f"{SETTER} MODE {chan} -l")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +l 50"
    mock_ircd.send(f":mock.test 324 TestBot {chan} +ntl 50")

    mock_ircd.send(f"{SETTER} MODE {chan} +l 10")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +l 50"


# ---------- A18: enforcebans kicks matching members ----------


def test_a18_enforcebans_kicks_banned_member(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(
        mock_ircd, "@TestBot @someop alice", chanmodes_324="+nt"
    )
    wait_onchan(tcl_bridge, "alice", chan)
    tcl_bridge.eval_ok(f"channel set {chan} +enforcebans")

    mock_ircd.send(f"{SETTER} MODE {chan} +b alice!*@*")
    kick = mock_ircd.drain_until(
        lambda line: line.startswith(f"KICK {chan} alice"), timeout=5.0
    )[-1]
    assert kick.startswith(f"KICK {chan} alice :"), kick


# ---------- A19: a ban matching the bot is reversed ----------


def test_a19_ban_on_bot_is_reversed(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)

    mock_ircd.send(f"{SETTER} MODE {chan} +b TestBot!*@*")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} -b TestBot!*@*"


# ---------- A20: sticky / non-dynamic bans are re-added ----------


def test_a20_sticky_ban_readded_after_unban(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)

    tcl_bridge.eval_ok(f'newchanban {chan} "stick!*@*" tester pinned 0 sticky')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +b stick!*@*"

    mock_ircd.send(f"{SETTER} MODE {chan} -b stick!*@*")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +b stick!*@*"


def test_a20_static_ban_readded_when_not_dynamic(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """-dynamicbans: userfile bans are kept set; an unban by another op is
    re-added."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    tcl_bridge.eval_ok(f"channel set {chan} -dynamicbans")

    tcl_bridge.eval_ok(f'newchanban {chan} "static!*@*" tester pinned 0')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +b static!*@*"

    mock_ircd.send(f"{SETTER} MODE {chan} -b static!*@*")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +b static!*@*"


# ---------- A21: nouser-list policy ----------


def test_a21_nouserexempts_removes_foreign_exempt(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    tcl_bridge.eval_ok(f"channel set {chan} -userexempts")

    mock_ircd.send(f"{SETTER} MODE {chan} +e foo!*@*")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} -e foo!*@*"


# ---------- A24: b/e/I list tracking ----------


def test_a24_list_tracking_numerics_and_live(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """List numerics (367/348/346) and live ±b/e/I MODE changes are both
    tracked in chanbans/chanexempts/chaninvites."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)

    mock_ircd.send(f":mock.test 367 TestBot {chan} pre1!*@* setter 12345")
    mock_ircd.send(f":mock.test 348 TestBot {chan} pre2!*@* setter 12345")
    mock_ircd.send(f":mock.test 346 TestBot {chan} pre3!*@* setter 12345")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'ischaninvite "pre3!*@*" "{chan}"') == "1",
        timeout=5.0,
        description="346 invite to be tracked",
    )
    assert tcl_bridge.eval_ok(f'ischanban "pre1!*@*" "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'ischanexempt "pre2!*@*" "{chan}"') == "1"

    mock_ircd.send(f"{SETTER} MODE {chan} +b live1!*@*")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'ischanban "live1!*@*" "{chan}"') == "1",
        timeout=5.0,
        description="+b to be tracked",
    )
    mock_ircd.send(f"{SETTER} MODE {chan} -b live1!*@*")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'ischanban "live1!*@*" "{chan}"') == "0",
        timeout=5.0,
        description="-b to be untracked",
    )
    # the numeric-sourced entries are unaffected
    assert tcl_bridge.eval_ok(f'ischanban "pre1!*@*" "{chan}"') == "1"


# ---------- A25: chanmode setting round-trip ----------


@pytest.mark.partyline
def test_a25_chanmode_roundtrip_and_persistence(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    tmp_eggdir,  # Path fixture from conftest
) -> None:
    """`channel set` and partyline `.chanset` both update chanmode;
    savechannels serializes it to the chanfile and the value survives a
    rehash (the chanfile is read after the conf's `channel add +nt`).

    Note: rehash *writes* the chanfile from memory before re-reading, so
    "change memory then rehash to restore" is untestable by design — the
    pinned contract is the save/survive round-trip.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    tcl_bridge.eval_ok(f'channel set {chan} chanmode "+mntk sekrit"')
    got = tcl_bridge.eval_ok(f"channel get {chan} chanmode")
    flags, _, args = got.partition(" ")
    assert set(flags.lstrip("+")) == set("mntk"), got
    assert args.strip() == "sekrit", got

    eggdrop_proc.send_partyline(f".chanset {chan} chanmode +imnt")
    wait_for(
        lambda: "i" in tcl_bridge.eval_ok(f"channel get {chan} chanmode"),
        timeout=5.0,
        description=".chanset chanmode to apply",
    )
    got = tcl_bridge.eval_ok(f"channel get {chan} chanmode")
    assert set(got.lstrip("+")) == set("imnt"), got

    tcl_bridge.eval_ok("savechannels")
    chanfile = (tmp_eggdir / "eggdrop.chan").read_text()
    m = re.search(r"chanmode (\S+)", chanfile)
    assert m, chanfile
    assert set(m.group(1).lstrip("+")) == set("imnt"), chanfile

    tcl_bridge.eval_ok("rehash")
    wait_for(
        lambda: set(
            tcl_bridge.eval_ok(f"channel get {chan} chanmode").lstrip("+")
        )
        == set("imnt"),
        timeout=5.0,
        description="chanmode to survive a rehash via the chanfile",
    )


# ---------- A26: partyline .op / .kickban ----------


@pytest.mark.partyline
def test_a26_partyline_op_and_kickban(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "alice", chan)

    eggdrop_proc.send_partyline(f".op alice {chan}")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +o alice"

    eggdrop_proc.send_partyline(f".kickban {chan} alice")
    banline = next_mode_line(mock_ircd, chan)
    assert " +b " in banline, banline
    kick = mock_ircd.drain_until(
        lambda line: line.startswith(f"KICK {chan} alice"), timeout=5.0
    )[-1]
    assert kick.startswith(f"KICK {chan} alice :"), kick


# ---------- A28: bind proc removing the channel mid-burst ----------


def test_a28_bind_mode_removing_channel_does_not_crash(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A `bind mode` proc that removes the channel mid-burst: the rest of
    the burst is dropped cleanly (modebind_refresh contract), no crash."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    tcl_bridge.eval_ok(
        f'proc test:remchan {{n u h c m v}} {{channel remove {chan}}}'
    )
    tcl_bridge.eval_ok("bind mode - * test:remchan")

    mock_ircd.send(f"{SETTER} MODE {chan} +bi bad!*@*")
    wait_for(
        lambda: chan not in tcl_bridge.eval_ok("channels").split(),
        timeout=5.0,
        description="channel to be removed by the bind proc",
    )
    eggdrop_proc.assert_alive()
    # The interpreter must still be responsive.
    assert tcl_bridge.eval_ok("expr {2+2}") == "4"
