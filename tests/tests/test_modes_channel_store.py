"""B4: channel mode store and explicit modes-known state."""

from __future__ import annotations

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd, MockIrcdError
from support.waiters import wait_for


def next_mode_line(mock_ircd: MockIrcd, chan: str, timeout: float = 5.0) -> str:
    return mock_ircd.drain_until(
        lambda line: line.startswith(f"MODE {chan} "), timeout=timeout
    )[-1]


def assert_no_mode_push(mock_ircd: MockIrcd, chan: str) -> None:
    try:
        line = mock_ircd.drain_until(
            lambda line: line.startswith((f"MODE {chan} +", f"MODE {chan} -")),
            timeout=1.25,
        )[-1]
    except MockIrcdError:
        return
    raise AssertionError(f"unexpected mode push before 324: {line}")


def test_b7_inbound_advertised_flag_appears_in_getchanmode(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    chanmodes = "beI,k,l,imnpstS"

    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={chanmodes}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    before = tcl_bridge.eval_ok(f'getchanmode "{chan}"')
    assert "S" not in before.split()[0]

    mock_ircd.send(f":someop!u@h MODE {chan} +S")
    eggdrop_proc.wait_for_log(r"mode change '\+S")

    wait_for(
        lambda: "S" in tcl_bridge.eval_ok(f'getchanmode "{chan}"').split()[0],
        timeout=5.0,
        description="+S to appear in getchanmode",
    )


def test_b7_324_arbitrary_flag_and_arg_modes_appear_in_getchanmode(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    chanmodes = "beI,k,lj,imnpstS"

    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={chanmodes}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":mock.test 324 TestBot {chan} +ntSj 3:5")
    wait_for(
        lambda: "3:5" in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=5.0,
        description="324 +ntSj to update generic mode store",
    )

    mode = tcl_bridge.eval_ok(f'getchanmode "{chan}"')
    parts = mode.split()
    assert len(parts) == 2, mode
    flags = parts[0]
    assert set("ntSj") <= set(flags), mode
    assert parts[1] == "3:5"
    assert tcl_bridge.eval_ok(f'dict get [getchanmodes "{chan}"] S') == ""
    assert tcl_bridge.eval_ok(f'dict get [getchanmodes "{chan}"] j') == "3:5"


def test_b4_modes_known_gate_delays_join_enforcement_until_324(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
) -> None:
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    assert_no_mode_push(mock_ircd, chan)

    mock_ircd.send(f":mock.test 324 TestBot {chan} +n")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +t"
