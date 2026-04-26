"""Converted from eggdrop-tests/eggdrop_tcl_passwdok.bats.

Tests for the `passwdok` Tcl command — verifies user passwords match
their stored value, with correct handling of the empty / dash sentinels.
"""

from __future__ import annotations

from support.bridge_client import BridgeClient


def test_passwdok_returns_1_when_password_matches(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("adduser foo")
    tcl_bridge.eval_ok("setuser foo PASS asdf")
    assert tcl_bridge.eval_ok("passwdok foo asdf") == "1"


def test_passwdok_returns_0_when_password_differs(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("adduser foo")
    tcl_bridge.eval_ok("setuser foo PASS asdf")
    assert tcl_bridge.eval_ok("passwdok foo notasdf") == "0"


def test_passwdok_returns_1_for_dash_when_user_has_no_password(
    tcl_bridge: BridgeClient,
) -> None:
    """A `-` password literal matches a user that has no password set."""
    tcl_bridge.eval_ok("adduser foo")
    tcl_bridge.eval_ok("setuser foo PASS {}")  # clear password
    assert tcl_bridge.eval_ok("passwdok foo -") == "1"


def test_passwdok_returns_0_for_dash_when_user_has_a_password(
    tcl_bridge: BridgeClient,
) -> None:
    tcl_bridge.eval_ok("adduser foo")
    tcl_bridge.eval_ok("setuser foo PASS asdf")
    assert tcl_bridge.eval_ok("passwdok foo -") == "0"


def test_passwdok_returns_0_for_empty_when_user_has_no_password(
    tcl_bridge: BridgeClient,
) -> None:
    tcl_bridge.eval_ok("adduser foo")
    tcl_bridge.eval_ok("setuser foo PASS {}")
    assert tcl_bridge.eval_ok('passwdok foo ""') == "0"


def test_passwdok_returns_0_for_empty_when_user_has_a_password(
    tcl_bridge: BridgeClient,
) -> None:
    tcl_bridge.eval_ok("adduser foo")
    tcl_bridge.eval_ok("setuser foo PASS asdf")
    assert tcl_bridge.eval_ok('passwdok foo ""') == "0"
