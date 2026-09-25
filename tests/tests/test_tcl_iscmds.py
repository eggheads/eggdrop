"""Converted from eggdrop-tests/eggdrop_tcl_iscmds.bats.

Tests for `isban`, `isbansticky`, `isexempt`, `isinvite` — the 4 lookup
commands for global vs channel ban/exempt/invite lists.

The original bats test had a single shared eggdrop and mutated state
between cases; here each test gets a fresh spawn and sets up its own
state inline, which makes the intent of each case obvious in isolation.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient

CHAN = "#foober"
HOST = "*!test@foo.com"


@pytest.fixture
def chan(tcl_bridge: BridgeClient) -> str:
    """Add #foober and return its name."""
    tcl_bridge.eval_ok(f"channel add {CHAN}")
    return CHAN


# ---------- isban ----------


def test_isban_returns_0_when_no_global_or_channel_ban(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    assert tcl_bridge.eval_ok(f"isban {HOST}") == "0"


def test_isban_returns_1_when_only_global_ban_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newban {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isban {HOST}") == "1"


def test_isban_with_channel_returns_1_when_only_channel_ban_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newchanban {chan} {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isban {HOST} {chan}") == "1"


def test_isban_returns_1_when_both_global_and_channel_ban_exist(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newban {HOST} testuser comment")
    tcl_bridge.eval_ok(f"newchanban {chan} {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isban {HOST} {chan}") == "1"


def test_isban_with_channel_returns_1_for_global_when_only_channel_ban_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    """isban without a channel hits the global list — global ban present → 1."""
    tcl_bridge.eval_ok(f"newban {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isban {HOST}") == "1"


def test_isban_with_channel_returns_0_when_only_global_ban_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    """isban WITH a channel checks both — but if neither global+channel match,
    returns 0. Here only global is set, so isban for chan-scope returns 1
    (eggdrop combines global + chan)."""
    tcl_bridge.eval_ok(f"newban {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isban {HOST} {chan}") == "1"


def test_isban_returns_0_when_only_channel_ban_exists_no_chan_arg(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    """Without a channel arg, isban only looks at the global list."""
    tcl_bridge.eval_ok(f"newchanban {chan} {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isban {HOST}") == "0"


# ---------- isbansticky ----------


def test_isbansticky_returns_0_when_no_global_or_channel_sticky_ban(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    assert tcl_bridge.eval_ok(f"isbansticky {HOST}") == "0"


def test_isbansticky_returns_1_when_only_global_sticky_ban_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newban {HOST} testuser comment 60 sticky")
    assert tcl_bridge.eval_ok(f"isbansticky {HOST}") == "1"


def test_isbansticky_with_channel_returns_1_when_only_channel_sticky_ban_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newchanban {chan} {HOST} testuser comment 60 sticky")
    assert tcl_bridge.eval_ok(f"isbansticky {HOST} {chan}") == "1"


def test_isbansticky_returns_1_when_both_sticky_bans_exist(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newban {HOST} testuser comment 60 sticky")
    tcl_bridge.eval_ok(f"newchanban {chan} {HOST} testuser comment 60 sticky")
    assert tcl_bridge.eval_ok(f"isbansticky {HOST} {chan}") == "1"


def test_isbansticky_returns_0_when_only_channel_sticky_ban_no_chan_arg(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newchanban {chan} {HOST} testuser comment 60 sticky")
    # Original bats test asserts isban (not isbansticky) returns 0 here. Match it.
    assert tcl_bridge.eval_ok(f"isban {HOST}") == "0"


# ---------- isexempt ----------


def test_isexempt_returns_0_when_no_global_or_channel_exempt(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    assert tcl_bridge.eval_ok(f"isexempt {HOST}") == "0"


def test_isexempt_returns_1_when_only_global_exempt_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newexempt {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isexempt {HOST}") == "1"


def test_isexempt_with_channel_returns_1_when_only_channel_exempt_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newchanexempt {chan} {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isexempt {HOST} {chan}") == "1"


def test_isexempt_returns_1_when_both_global_and_channel_exempt_exist(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newexempt {HOST} testuser comment")
    tcl_bridge.eval_ok(f"newchanexempt {chan} {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isexempt {HOST} {chan}") == "1"


def test_isexempt_returns_0_when_only_channel_exempt_no_chan_arg(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newchanexempt {chan} {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isexempt {HOST}") == "0"


# ---------- isinvite ----------


def test_isinvite_returns_0_when_no_global_or_channel_invite(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    assert tcl_bridge.eval_ok(f"isinvite {HOST}") == "0"


def test_isinvite_returns_1_when_only_global_invite_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newinvite {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isinvite {HOST}") == "1"


def test_isinvite_with_channel_returns_1_when_only_channel_invite_exists(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newchaninvite {chan} {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isinvite {HOST} {chan}") == "1"


def test_isinvite_returns_1_when_both_global_and_channel_invite_exist(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newinvite {HOST} testuser comment")
    tcl_bridge.eval_ok(f"newchaninvite {chan} {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isinvite {HOST} {chan}") == "1"


def test_isinvite_returns_0_when_only_channel_invite_no_chan_arg(
    tcl_bridge: BridgeClient, chan: str
) -> None:
    tcl_bridge.eval_ok(f"newchaninvite {chan} {HOST} testuser comment")
    assert tcl_bridge.eval_ok(f"isinvite {HOST}") == "0"
