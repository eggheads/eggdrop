"""B5: generic gotmode dispatch and generic list tracking."""

from __future__ import annotations

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    get_mode_log,
    install_mode_log,
    wait_onchan,
)
from support.mock_ircd import MockIrcd
from support.waiters import wait_for

SETTER = ":someop!su@sh.example.com"


def next_mode_line(mock_ircd: MockIrcd, chan: str, timeout: float = 5.0) -> str:
    return mock_ircd.drain_until(
        lambda line: line.startswith(f"MODE {chan} "), timeout=timeout
    )[-1]


def dict_exists(tcl_bridge: BridgeClient, chan: str, mode: str) -> str:
    return tcl_bridge.eval_ok(f'dict exists [getchanmodes "{chan}"] {mode}')


def dict_get(tcl_bridge: BridgeClient, chan: str, mode: str) -> str:
    return tcl_bridge.eval_ok(f'dict get [getchanmodes "{chan}"] {mode}')


def test_b5_advertised_flag_is_tracked_and_fires_bind(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beI,k,l,imnpstS"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    install_mode_log(tcl_bridge)

    mock_ircd.send(f"{SETTER} MODE {chan} +S")

    wait_for(
        lambda: dict_exists(tcl_bridge, chan, "S") == "1",
        timeout=5.0,
        description="+S to be tracked in getchanmodes",
    )
    assert dict_get(tcl_bridge, chan, "S") == ""
    assert "+S||0" in get_mode_log(tcl_bridge)


def test_b5_generic_limit_mode_is_tracked_with_arg(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beI,k,lj,imnpstS"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    install_mode_log(tcl_bridge)

    mock_ircd.send(f"{SETTER} MODE {chan} +j 3:5")

    wait_for(
        lambda: dict_exists(tcl_bridge, chan, "j") == "1",
        timeout=5.0,
        description="+j arg to be tracked in getchanmodes",
    )
    assert dict_get(tcl_bridge, chan, "j") == "3:5"
    assert "+j|3:5|0" in get_mode_log(tcl_bridge)


def test_b5_quiet_list_tracks_generic_list_not_legacy_flag(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beIq,k,l,imnpst"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)
    install_mode_log(tcl_bridge)

    mock_ircd.send(f"{SETTER} MODE {chan} +q *!*@x")
    wait_for(
        lambda: "*!*@x" in tcl_bridge.eval_ok(f'chanmodelist "{chan}" q').split(),
        timeout=5.0,
        description="+q mask to enter generic list store",
    )

    assert dict_exists(tcl_bridge, chan, "q") == "0"
    assert "q" not in tcl_bridge.eval_ok(f'getchanmode "{chan}"').split()[0]
    assert "+q|*!*@x|0" in get_mode_log(tcl_bridge)

    mock_ircd.send(f"{SETTER} MODE {chan} -q *!*@x")
    wait_for(
        lambda: "*!*@x"
        not in tcl_bridge.eval_ok(f'chanmodelist "{chan}" q').split(),
        timeout=5.0,
        description="-q mask to leave generic list store",
    )


def test_b5_flag_q_network_tracks_q_as_flag(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beI,k,l,imnpqst"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)

    mock_ircd.send(f"{SETTER} MODE {chan} +q")

    wait_for(
        lambda: dict_exists(tcl_bridge, chan, "q") == "1",
        timeout=5.0,
        description="+q flag to be tracked in getchanmodes",
    )
    assert dict_get(tcl_bridge, chan, "q") == ""


def test_b5_chanmode_protected_generic_flag_is_reversed(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beI,k,l,imnpstS"])
    chan = drive_join_with_names(
        mock_ircd, "@TestBot @someop", chanmodes_324="+ntS"
    )
    wait_onchan(tcl_bridge, "someop", chan)
    tcl_bridge.eval_ok(f'channel set {chan} chanmode "+ntS"')

    mock_ircd.send(f":mock.test MODE {chan} -S")

    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +S"
