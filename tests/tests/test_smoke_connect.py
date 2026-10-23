"""End-to-end smoke test: spawn Eggdrop, register, join, introspect via bridge."""

from __future__ import annotations

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd


def test_bridge_alive(tcl_bridge: BridgeClient) -> None:
    """Eggdrop boots, the bridge listens, Tcl evaluates."""
    assert tcl_bridge.eval_ok("expr {2 + 2}") == "4"
    assert tcl_bridge.eval_ok("info patchlevel")  # non-empty
    assert tcl_bridge.eval_ok("set ::nick") == "TestBot"


def test_connect_register_join_and_introspect(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Full happy path: register, join, introspect."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    assert chan == "#test"

    # Use lsearch in Tcl so list quoting (curly-brace wrapping of names that
    # start with '#') doesn't trip us up.
    assert tcl_bridge.eval_ok('expr {[lsearch [channels] "#test"] >= 0}') == "1"

    # Owner host from the rendered userfile is *!*@127.0.0.1.
    hosts = tcl_bridge.eval_ok("getuser owner HOSTS")
    assert "127.0.0.1" in hosts
