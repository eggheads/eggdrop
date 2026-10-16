"""B8: verbatim chanmode storage and generic protection."""

from __future__ import annotations

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    wait_onchan,
)
from support.mock_ircd import MockIrcd, MockIrcdError

SETTER = ":someop!su@sh.example.com"


def next_mode_line(mock_ircd: MockIrcd, chan: str, timeout: float = 5.0) -> str:
    return mock_ircd.drain_until(
        lambda line: line.startswith(f"MODE {chan} "), timeout=timeout
    )[-1]


def assert_no_mode_push(mock_ircd: MockIrcd, chan: str) -> None:
    try:
        line = mock_ircd.drain_until(
            lambda line: line.startswith((f"MODE {chan} +", f"MODE {chan} -")),
            timeout=1.0,
        )[-1]
    except MockIrcdError:
        return
    raise AssertionError(f"unexpected mode push: {line}")


def mode_letters(line: str) -> set[str]:
    return set(line.split()[2].lstrip("+-"))


def test_b8_chanmode_stored_verbatim_preconnect_then_enforced_after_005(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    tcl_bridge.eval_ok('channel set #test chanmode "+zS"')
    assert tcl_bridge.eval_ok("channel get #test chanmode") == "+zS"
    eggdrop_proc.wait_for_log(r"chanmode for #test contains mode \+z: unknown")

    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beI,k,l,imnpstzS"])
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+n")

    line = next_mode_line(mock_ircd, chan)
    assert mode_letters(line) == set("zS"), line


def test_b8_generic_limit_chanmode_is_enforced_with_argument(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beI,k,lj,imnpst"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)

    tcl_bridge.eval_ok(f'channel set {chan} chanmode "+ntj 3:5"')
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +j 3:5"

    mock_ircd.send(f"{SETTER} MODE {chan} +j 1:2")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +j 3:5"


def test_b8_list_and_prefix_modes_in_chanmode_warn_and_do_not_enforce(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beIq,k,l,imnpst"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)

    tcl_bridge.eval_ok(f'channel set {chan} chanmode "+qo *!*@x someop"')
    assert tcl_bridge.eval_ok(f"channel get {chan} chanmode") == "+qo *!*@x someop"
    eggdrop_proc.wait_for_log(r"mode \+q: list modes are not enforceable")
    eggdrop_proc.wait_for_log(r"mode \+o: prefix modes are not enforceable")
    assert_no_mode_push(mock_ircd, chan)


def test_b8_isupport_change_rederives_chanmode_enforcement(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd, isupport_tokens=["CHANMODES=beI,k,l,imnpst"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @someop", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "someop", chan)

    tcl_bridge.eval_ok(f'channel set {chan} chanmode "+ntj 3:5"')
    eggdrop_proc.wait_for_log(r"chanmode for #test contains mode \+j: unknown")
    assert_no_mode_push(mock_ircd, chan)

    mock_ircd.send(":mock.test 005 TestBot CHANMODES=beI,k,lj,imnpst :are supported")
    assert next_mode_line(mock_ircd, chan) == f"MODE {chan} +j 3:5"
