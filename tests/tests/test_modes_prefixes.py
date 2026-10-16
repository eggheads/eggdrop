"""B2: arbitrary PREFIX member storage and multi-prefix negotiation."""

from __future__ import annotations

import contextlib
from collections.abc import Iterator

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    get_mode_log,
    install_mode_log,
    wait_for_isupport,
)
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


@pytest.fixture
def mock_ircd() -> Iterator[MockIrcd]:
    ircd = MockIrcd(advertised_caps=["multi-prefix"]).start()
    try:
        yield ircd
    finally:
        with contextlib.suppress(Exception):
            ircd.stop()


def test_b2_multi_prefix_is_requested(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)
    wait_for(
        lambda: "multi-prefix" in tcl_bridge.eval_ok("cap enabled").split(),
        timeout=5.0,
        description="multi-prefix cap to be enabled",
    )


def test_b2_whox_prefixes_keep_isop_literal_o(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    prefix = "(qaohv)~&@%+"

    drive_registration(mock_ircd, isupport_tokens=["WHOX", f"PREFIX={prefix}"])
    wait_for_isupport(tcl_bridge, "PREFIX", prefix)
    wait_for(
        lambda: "multi-prefix" in tcl_bridge.eval_ok("cap enabled").split(),
        timeout=5.0,
        description="multi-prefix cap to be enabled",
    )

    chan = drive_join_with_names(mock_ircd, "@TestBot ~owner ~@opowner +voice")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan opowner "{chan}"') == "1",
        timeout=5.0,
        description="opowner to appear in chanlist",
    )

    assert tcl_bridge.eval_ok(f'isop owner "{chan}"') == "0"
    assert tcl_bridge.eval_ok(f'isop opowner "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isvoice voice "{chan}"') == "1"


def test_b2_opchars_warns_once_and_does_not_grant_owner_op(
    eggdrop_config,
    request: pytest.FixtureRequest,
) -> None:
    eggdrop_config.render(extra_tcl='set opchars "~&@"\nset opchars "@"\n')
    mock_ircd: MockIrcd = request.getfixturevalue("mock_ircd")
    eggdrop_proc: EggdropProc = request.getfixturevalue("eggdrop_proc")
    tcl_bridge: BridgeClient = request.getfixturevalue("tcl_bridge")
    prefix = "(qaohv)~&@%+"

    wait_for(
        lambda: eggdrop_proc.log_path.read_text().count("opchars is deprecated") == 1,
        timeout=5.0,
        description="single opchars deprecation warning",
    )

    drive_registration(mock_ircd, isupport_tokens=["WHOX", f"PREFIX={prefix}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot ~owner")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan owner "{chan}"') == "1",
        timeout=5.0,
        description="owner to appear in chanlist",
    )
    assert tcl_bridge.eval_ok(f'isop owner "{chan}"') == "0"


def test_b2_generic_prefix_mode_consumes_arg_and_fires_bind(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    prefix = "(qov)~@+"

    drive_registration(mock_ircd, isupport_tokens=[f"PREFIX={prefix}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot owner voiced")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan voiced "{chan}"') == "1",
        timeout=5.0,
        description="voiced to appear in chanlist",
    )
    install_mode_log(tcl_bridge)

    mock_ircd.send(f":oper!u@h MODE {chan} +qv owner voiced")

    wait_for(
        lambda: "+q|owner|0" in get_mode_log(tcl_bridge),
        timeout=5.0,
        description="generic +q bind mode entry",
    )
    assert tcl_bridge.eval_ok(f'isvoice voiced "{chan}"') == "1"
