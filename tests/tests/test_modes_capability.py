"""B3: rank-based channel mode capability gates."""

from __future__ import annotations

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    wait_onchan,
)
from support.mock_ircd import MockIrcd


def next_mode_line(mock_ircd: MockIrcd, chan: str, timeout: float = 5.0) -> str:
    return mock_ircd.drain_until(
        lambda line: line.startswith(f"MODE {chan} "), timeout=timeout
    )[-1]


def mode_letters(line: str) -> str:
    return line.split()[2].replace("+", "").replace("-", "")


def test_b3_halfop_can_set_lower_prefix_and_nonprefix_modes(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["PREFIX=(ohv)@%+"])
    chan = drive_join_with_names(mock_ircd, "%TestBot alice", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "alice", chan)

    tcl_bridge.eval_ok(f'pushmode "{chan}" +v alice')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +v alice"

    tcl_bridge.eval_ok(f'pushmode "{chan}" +o alice')
    tcl_bridge.eval_ok(f'pushmode "{chan}" +h alice')
    tcl_bridge.eval_ok(f'pushmode "{chan}" +b "half!*@*"')
    tcl_bridge.eval_ok(f'pushmode "{chan}" +i')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    line = next_mode_line(mock_ircd, chan)
    letters = mode_letters(line)

    assert "o" not in letters, line
    assert "h" not in letters, line
    assert "b" in letters, line
    assert "i" in letters, line
    assert line.split()[3:] == ["half!*@*"], line


def test_b3_op_can_set_and_unset_op_self_rank(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["PREFIX=(ohv)@%+"])
    chan = drive_join_with_names(
        mock_ircd, "@TestBot alice @opped", chanmodes_324="+nt"
    )
    wait_onchan(tcl_bridge, "alice", chan)
    wait_onchan(tcl_bridge, "opped", chan)

    tcl_bridge.eval_ok(f'pushmode "{chan}" +o alice')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +o alice"

    tcl_bridge.eval_ok(f'pushmode "{chan}" -o opped')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} -o opped"


def test_b3_halfop_quiet_list_mode_is_not_blocked_by_removed_q_denylist(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(
        mock_ircd,
        isupport_tokens=["PREFIX=(ohv)@%+", "CHANMODES=beIq,k,l,imnpst"],
    )
    chan = drive_join_with_names(mock_ircd, "%TestBot", chanmodes_324="+nt")

    tcl_bridge.eval_ok(f'pushmode "{chan}" +q "quiet!*@*"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    line = next_mode_line(mock_ircd, chan)
    assert "q" in mode_letters(line), line
