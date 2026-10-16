"""Characterization tests for the outbound mode queue (arbmodes3 step 0).

ARCHITECTURE.md test plan A6-A9: pushmode/flushmode wire output, dedup,
the k/l special slots, prevent-mixing, and line splitting. These pin the
exact wire behaviour the step-6 queue rewrite must reproduce.

All joins pass `chanmodes_324="+nt"` so the bot's own chanmode
enforcement leaves the queue clean (see drive_join_with_names docstring).
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient, BridgeError
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    wait_for_isupport,
)
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


def next_mode_line(mock_ircd: MockIrcd, chan: str, timeout: float = 5.0) -> str:
    return mock_ircd.drain_until(
        lambda line: line.startswith(f"MODE {chan} "), timeout=timeout
    )[-1]


# ---------- A6: classic pushmode + dedup ----------


def test_a6_pushmode_op_and_ban(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice", chanmodes_324="+nt")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1",
        timeout=5.0,
        description="alice on channel",
    )

    tcl_bridge.eval_ok(f'pushmode "{chan}" +o alice')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +o alice"

    tcl_bridge.eval_ok(f'pushmode "{chan}" +b "bad!*@*"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +b bad!*@*"


def test_a6_pushmode_plus_o_dedup_when_already_op(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """pushmode +o for a member the bot already sees as op is suppressed
    (the queue also dedups while a +o is pending un-echoed; the
    already-op variant is asserted here because the state is observable).
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @alice", chanmodes_324="+nt")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isop alice "{chan}"') == "1",
        timeout=5.0,
        description="alice op from WHO",
    )

    tcl_bridge.eval_ok(f'pushmode "{chan}" +o alice')
    tcl_bridge.eval_ok(f'pushmode "{chan}" +b "sentinel!*@*"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    line = next_mode_line(mock_ircd, chan)
    assert line == f"MODE {chan} +b sentinel!*@*", line


# ---------- A7: key/limit ride dedicated slots but emit normally ----------


def test_a7_pushmode_key_and_limit_wire_output(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    tcl_bridge.eval_ok(f'pushmode "{chan}" +k secret')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +k secret"

    tcl_bridge.eval_ok(f'pushmode "{chan}" +l 42')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +l 42"

    tcl_bridge.eval_ok(f'pushmode "{chan}" -k secret')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} -k secret"


def test_a7_pushmode_key_limit_combined_with_flags(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Flags + key + limit queued together come out as one line with args
    in k-before-l order (the historic flush_mode emission order)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    tcl_bridge.eval_ok(f'pushmode "{chan}" +i')
    tcl_bridge.eval_ok(f'pushmode "{chan}" +k secret')
    tcl_bridge.eval_ok(f'pushmode "{chan}" +l 42')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    line = next_mode_line(mock_ircd, chan)
    words = line.split()
    modes, args = words[2], words[3:]
    assert modes.startswith("+"), line
    assert set(modes.lstrip("+")) == set("ikl"), line
    # key arg precedes limit arg in the current emission order.
    assert args == ["secret", "42"], line


# ---------- A8: prevent-mixing splits e/I from other modes ----------


def test_a8_prevent_mixing_splits_exempt_from_ban(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """With prevent-mixing (default 1), queueing +b then +e flushes the +b
    immediately and the +e separately: two MODE lines, never one."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    tcl_bridge.eval_ok(f'pushmode "{chan}" +b "m1!*@*"')
    tcl_bridge.eval_ok(f'pushmode "{chan}" +e "m2!*@*"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    first = next_mode_line(mock_ircd, chan)
    second = next_mode_line(mock_ircd, chan)
    assert first == f"MODE {chan} +b m1!*@*", first
    assert second == f"MODE {chan} +e m2!*@*", second


# ---------- A9: line splitting at modesperline ----------


def test_a9_line_splitting_modes_4(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """MODES=4: six queued bans go out as a 4-mode line (auto-flushed when
    the queue fills) followed by a 2-mode line, args aligned to letters."""
    drive_registration(mock_ircd, isupport_tokens=["MODES=4"])
    wait_for_isupport(tcl_bridge, "MODES", "4")
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    for i in range(6):
        tcl_bridge.eval_ok(f'pushmode "{chan}" +b "m{i}!*@*"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    first = next_mode_line(mock_ircd, chan)
    second = next_mode_line(mock_ircd, chan)
    assert first == f"MODE {chan} +bbbb m0!*@* m1!*@* m2!*@* m3!*@*", first
    assert second == f"MODE {chan} +bb m4!*@* m5!*@*", second


# ---------- B6: arbitrary queue and pushmode validation ----------


def test_b6_pushmode_generic_limit_mode_reaches_wire(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    chanmodes = "beI,k,lj,imnpst"
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={chanmodes}"])
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    tcl_bridge.eval_ok(f'pushmode "{chan}" +j 3:5')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +j 3:5"


def test_b6_push_order_preserves_interleaved_signs(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    mock_ircd.send(f":mock.test MODE {chan} +b b!*@*")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'ischanban "b!*@*" "{chan}"') == "1",
        timeout=5.0,
        description="server ban tracked",
    )

    tcl_bridge.eval_ok(f'pushmode "{chan}" +b "a!*@*"')
    tcl_bridge.eval_ok(f'pushmode "{chan}" -b "b!*@*"')
    tcl_bridge.eval_ok(f'pushmode "{chan}" +b "c!*@*"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +b-b+b a!*@* b!*@* c!*@*"


def test_b6_pushmode_unknown_mode_errors(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    with pytest.raises(BridgeError, match=r"unknown mode: \+Z"):
        tcl_bridge.eval_ok(f'pushmode "{chan}" +Z')


def test_b6_pushmode_argument_validation_errors(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    chanmodes = "beI,k,lj,imnpst"
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={chanmodes}"])
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    with pytest.raises(BridgeError, match=r"missing argument for mode \+j"):
        tcl_bridge.eval_ok(f'pushmode "{chan}" +j')
    with pytest.raises(BridgeError, match=r"excess argument for mode \+i"):
        tcl_bridge.eval_ok(f'pushmode "{chan}" +i stray')


def test_b6_byte_budget_splits_long_masks_without_truncation(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["MODES=20"])
    wait_for_isupport(tcl_bridge, "MODES", "20")
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")
    masks = [f"m{i}{'x' * 60}!*@example.test" for i in range(12)]

    for mask in masks:
        tcl_bridge.eval_ok(f'pushmode "{chan}" +b "{mask}"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    seen: list[str] = []
    lines: list[str] = []
    while len(seen) < len(masks):
        line = next_mode_line(mock_ircd, chan)
        lines.append(line)
        assert len(line) < 512, line
        words = line.split()
        modes = words[2].replace("+", "").replace("-", "")
        args = words[3:]
        assert len(args) == len(modes), line
        seen.extend(args)

    assert len(lines) > 1
    assert seen == masks
