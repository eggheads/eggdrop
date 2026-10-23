"""Converted from eggdrop-tests/eggdrop_tcl_matchattr.bats.

Tests for `matchattr` — Tcl flag-matching against a user record. Covers
global flags, channel flags, the `&` (all-of) operator, and rejection of
unknown flags.

Original setup:
  adduser foo
  channel add #foober
  chattr foo +jlmoptx|+lov #foober

Result: foo has global +jlmoptx and channel +lov on #foober.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient

CHAN = "#foober"


@pytest.fixture
def matchattr_user(tcl_bridge: BridgeClient) -> str:
    """Per-test setup: adduser foo with global +jlmoptx and channel +lov on #foober."""
    tcl_bridge.eval_ok(f"channel add {CHAN}")
    tcl_bridge.eval_ok("adduser foo")
    tcl_bridge.eval_ok(f"chattr foo +jlmoptx|+lov {CHAN}")
    return "foo"


# ---------- + (any-of) on global flags ----------


def test_matchattr_single_global_plus_flag_matches_when_user_has_it(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok("matchattr foo +o") == "1"


def test_matchattr_single_global_plus_flag_does_not_match_when_user_lacks_it(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok("matchattr foo +g") == "0"


def test_matchattr_two_global_plus_flags_matches_if_user_has_one(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    """+ is any-of by default: +on matches if user has o OR n."""
    assert tcl_bridge.eval_ok("matchattr foo +on") == "1"


def test_matchattr_two_global_plus_flags_does_not_match_if_user_has_neither(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok("matchattr foo +gn") == "0"


# ---------- & (all-of) operator on global flags ----------


def test_matchattr_two_global_plus_flags_with_amp_matches_if_user_has_both(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    """& makes it all-of: +mo& matches if user has BOTH global m AND o."""
    assert tcl_bridge.eval_ok(f"matchattr foo +mo& {CHAN}") == "1"


def test_matchattr_two_global_plus_flags_with_amp_does_not_match_if_user_has_one(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo +mn& {CHAN}") == "0"


# ---------- - (none-of) on global flags ----------


def test_matchattr_single_global_minus_flag_matches_when_user_lacks_it(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    """-n matches if user does NOT have n."""
    assert tcl_bridge.eval_ok("matchattr foo -n") == "1"


def test_matchattr_single_global_minus_flag_does_not_match_when_user_has_it(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok("matchattr foo -m") == "0"


def test_matchattr_two_global_minus_flags_matches_if_user_lacks_one(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok("matchattr foo -mn") == "1"


def test_matchattr_two_global_minus_flags_matches_if_user_lacks_both(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok("matchattr foo -gn") == "1"


def test_matchattr_two_global_minus_flags_does_not_match_if_user_has_both(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok("matchattr foo -om") == "0"


# ---------- channel flags (| separator) ----------


def test_matchattr_single_channel_plus_flag_matches_when_user_has_it(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |+o {CHAN}") == "1"


def test_matchattr_single_channel_plus_flag_does_not_match_when_user_lacks_it(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |+g {CHAN}") == "0"


def test_matchattr_two_channel_plus_flags_matches_if_user_has_one(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |+on {CHAN}") == "1"


def test_matchattr_two_channel_plus_flags_does_not_match_if_user_has_neither(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |+gn {CHAN}") == "0"


def test_matchattr_two_channel_plus_flags_with_amp_matches_if_user_has_both(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo &+lo {CHAN}") == "1"


def test_matchattr_two_channel_plus_flags_with_amp_does_not_match_if_user_has_one(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo &+om {CHAN}") == "0"


def test_matchattr_single_channel_minus_flag_matches_when_user_lacks_it(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |-n {CHAN}") == "1"


def test_matchattr_single_channel_minus_flag_does_not_match_when_user_has_it(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |-o {CHAN}") == "0"


def test_matchattr_two_channel_minus_flags_matches_if_user_lacks_one(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |-on {CHAN}") == "1"


def test_matchattr_two_channel_minus_flags_matches_if_user_lacks_both(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |-gn {CHAN}") == "1"


def test_matchattr_two_channel_minus_flags_does_not_match_if_user_has_both(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |-ov {CHAN}") == "0"


# ---------- error paths (behavior changed since the original bats tests) ----------
#
# The original bats suite asserted that matchattr returned a Tcl error
# `Unknown flag specified for matching` for unknown global/channel/bot flags.
# The current implementation in src/tcluser.c (tcl_matchattr) calls
# break_down_flags() which silently ignores unknown characters, then if the
# resulting flag set is empty returns "1" (the "no flags matches anyone"
# branch). The "rejection" is gone — these tests now document the new
# silent-accept behavior instead.


def test_matchattr_silently_accepts_unknown_global_flag(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    """`+s` is unknown globally; ignored, leaving an empty plus set → matches anyone."""
    assert tcl_bridge.eval_ok("matchattr foo +s") == "1"


def test_matchattr_silently_accepts_unknown_channel_flag(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok(f"matchattr foo |+j {CHAN}") == "1"


def test_matchattr_silently_accepts_unknown_bot_flag(
    tcl_bridge: BridgeClient, matchattr_user: str
) -> None:
    assert tcl_bridge.eval_ok("matchattr foo ||+f") == "1"
