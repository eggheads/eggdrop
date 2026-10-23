"""Converted from eggdrop-tests/eggdrop_chanset_inputvalidation.bats.

Input validation for `channel set <chan> flood-deop X:Y` — the X:Y format
parser must accept positive integer pairs, reject malformed input, and
treat the all-zero forms as "off"."""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient

CHAN = "#eggtest"


@pytest.fixture
def chan(tcl_bridge: BridgeClient) -> str:
    tcl_bridge.eval_ok(f"channel add {CHAN}")
    return CHAN


def test_flood_deop_accepts_valid_x_colon_y(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"channel set {chan} flood-deop 253:42")
    info = tcl_bridge.eval_ok(f"channel info {chan}")
    assert "253:42" in info


def test_flood_deop_rejects_non_integer_messages(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    """Letters in either field of X:Y are rejected."""
    for bad in ("4:D", "2a:4", "2:4a"):
        ok, result = tcl_bridge.eval(f"channel set {chan} flood-deop {bad}")
        assert not ok, f"{bad!r} unexpectedly accepted"
        assert "values must be integers >= 0" in result


def test_flood_deop_rejects_single_value(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    """A bare integer (no colon) is rejected — must be X:Y."""
    ok, result = tcl_bridge.eval(f"channel set {chan} flood-deop 4")
    assert not ok
    assert "flood value must be in X:Y format" in result


def test_flood_deop_rejects_invalid_format(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    """`:X`, `X:`, and three-section `X:Y:Z` are all rejected."""
    for bad in (":3", "3:", "53:75:86"):
        ok, result = tcl_bridge.eval(f"channel set {chan} flood-deop {bad}")
        assert not ok, f"{bad!r} unexpectedly accepted"
        assert "flood value must be in X:Y format" in result


def test_flood_deop_rejects_negative_numbers(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    ok, result = tcl_bridge.eval(f"channel set {chan} flood-deop -1:8")
    assert not ok
    assert "values must be integers >= 0" in result


def test_flood_deop_zero_clears_setting(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    """Bare `0` resets the setting to `0:0`."""
    tcl_bridge.eval_ok(f"channel set {chan} flood-deop 6:9")
    tcl_bridge.eval_ok(f"channel set {chan} flood-deop 0")
    info = tcl_bridge.eval_ok(f"channel info {chan}")
    assert "0:0" in info


def test_flood_deop_zero_zero_clears_setting(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"channel set {chan} flood-deop 6:9")
    tcl_bridge.eval_ok(f"channel set {chan} flood-deop 0:0")
    info = tcl_bridge.eval_ok(f"channel info {chan}")
    assert "0:0" in info


def test_flood_deop_n_zero_keeps_n(tcl_bridge: BridgeClient, chan: str) -> None:
    """`N:0` is a valid (degenerate) setting and is stored verbatim."""
    tcl_bridge.eval_ok(f"channel set {chan} flood-deop 6:9")
    tcl_bridge.eval_ok(f"channel set {chan} flood-deop 9:0")
    info = tcl_bridge.eval_ok(f"channel info {chan}")
    assert "9:0" in info


def test_flood_deop_zero_n_keeps_n(tcl_bridge: BridgeClient, chan: str) -> None:
    tcl_bridge.eval_ok(f"channel set {chan} flood-deop 6:9")
    tcl_bridge.eval_ok(f"channel set {chan} flood-deop 0:8")
    info = tcl_bridge.eval_ok(f"channel info {chan}")
    assert "0:8" in info
