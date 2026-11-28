"""Converted from eggdrop-tests/eggdrop_tcl_addbot.bats.

Tests for the `addbot` Tcl command — adding a bot record with various
address/port formats. Verifies the result is the expected `botaddr`
triple `host port port` (or `host botport userport`).

Skipped from the original bats: the two cases that required a second
non-IPv6 eggdrop instance to verify rejection (port < 1, port > 65535).
Those need a multi-bot harness which is out of scope for this framework.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient

DEFAULT_PORT = "3333"  # default-port set in the templated config
BOT = "testbot"


@pytest.fixture
def ipv6_required(tcl_bridge: BridgeClient) -> None:
    """Skip the test if Eggdrop wasn't compiled with IPv6 support."""
    # `info procs` won't work; check for IPv6 by trying an IPv6 add and seeing
    # if it succeeds. A simpler approach: peek at `set prefer-ipv6` (only set
    # when the build has IPv6). Use the actual `addbot` smoke test as the
    # truthy probe — addbot returns "0" if IPv6 isn't supported.
    tcl_bridge.eval_ok("deluser ipv6probe")
    if tcl_bridge.eval_ok("addbot ipv6probe ::1") != "1":
        pytest.skip("Eggdrop built without IPv6 support")
    tcl_bridge.eval_ok("deluser ipv6probe")


# ---------- legacy 'addbot handle address ?botport ?userport??' format ----------


def test_addbot_handle_ipv4_uses_default_port(tcl_bridge: BridgeClient) -> None:
    assert tcl_bridge.eval_ok(f"addbot {BOT} 1.1.1.1") == "1"
    assert (
        tcl_bridge.eval_ok(f"getuser {BOT} botaddr")
        == f"1.1.1.1 {DEFAULT_PORT} {DEFAULT_PORT}"
    )


def test_addbot_handle_ipv6_uses_default_port(
    tcl_bridge: BridgeClient, ipv6_required: None
) -> None:
    assert tcl_bridge.eval_ok(f"addbot {BOT} fe80::69ec:cfe4:81de:4fe5") == "1"
    assert (
        tcl_bridge.eval_ok(f"getuser {BOT} botaddr")
        == f"fe80::69ec:cfe4:81de:4fe5 {DEFAULT_PORT} {DEFAULT_PORT}"
    )


def test_addbot_handle_ipv4_with_botport(tcl_bridge: BridgeClient) -> None:
    assert tcl_bridge.eval_ok(f"addbot {BOT} 1.1.1.1 5555") == "1"
    assert tcl_bridge.eval_ok(f"getuser {BOT} botaddr") == "1.1.1.1 5555 5555"


def test_addbot_handle_ipv6_with_botport(
    tcl_bridge: BridgeClient, ipv6_required: None
) -> None:
    assert tcl_bridge.eval_ok(f"addbot {BOT} fe80::69ec:cfe4:81de:4fe5 6666") == "1"
    assert (
        tcl_bridge.eval_ok(f"getuser {BOT} botaddr")
        == "fe80::69ec:cfe4:81de:4fe5 6666 6666"
    )


def test_addbot_handle_ipv4_with_botport_and_userport(
    tcl_bridge: BridgeClient,
) -> None:
    assert tcl_bridge.eval_ok(f"addbot {BOT} 1.1.1.1 5555 6666") == "1"
    assert tcl_bridge.eval_ok(f"getuser {BOT} botaddr") == "1.1.1.1 5555 6666"


def test_addbot_handle_ipv6_with_botport_and_userport(
    tcl_bridge: BridgeClient, ipv6_required: None
) -> None:
    assert (
        tcl_bridge.eval_ok(f"addbot {BOT} fe80::69ec:cfe4:81de:4fe5 6666 7777")
        == "1"
    )
    assert (
        tcl_bridge.eval_ok(f"getuser {BOT} botaddr")
        == "fe80::69ec:cfe4:81de:4fe5 6666 7777"
    )


# ---------- ipv4:port packed format ----------


def test_addbot_handle_ipv4_colon_port(tcl_bridge: BridgeClient) -> None:
    assert tcl_bridge.eval_ok(f"addbot {BOT} 1.1.1.1:4444") == "1"
    assert tcl_bridge.eval_ok(f"getuser {BOT} botaddr") == "1.1.1.1 4444 4444"


def test_addbot_with_packed_address_ignores_extra_args(
    tcl_bridge: BridgeClient,
) -> None:
    """When the address arg uses `:`/`/` separators, extra positional args
    are ignored (the packed form is canonical)."""
    assert tcl_bridge.eval_ok(f"addbot {BOT} 1.1.1.1:4444/5555 6666") == "1"
    assert tcl_bridge.eval_ok(f"getuser {BOT} botaddr") == "1.1.1.1 4444 5555"


# ---------- bracketed IPv6 ----------


def test_addbot_bracketed_ipv6_with_colon_botport(
    tcl_bridge: BridgeClient, ipv6_required: None
) -> None:
    assert (
        tcl_bridge.eval_ok(f"addbot {BOT} \\[fe80::69ec:cfe4:81de:4fe5\\]:4444")
        == "1"
    )
    assert (
        tcl_bridge.eval_ok(f"getuser {BOT} botaddr")
        == "fe80::69ec:cfe4:81de:4fe5 4444 4444"
    )


def test_addbot_bracketed_ipv6_with_colon_botport_slash_userport(
    tcl_bridge: BridgeClient, ipv6_required: None
) -> None:
    assert (
        tcl_bridge.eval_ok(
            f"addbot {BOT} \\[fe80::69ec:cfe4:81de:4fe5\\]:4444/5555"
        )
        == "1"
    )
    assert (
        tcl_bridge.eval_ok(f"getuser {BOT} botaddr")
        == "fe80::69ec:cfe4:81de:4fe5 4444 5555"
    )


def test_addbot_bracketed_ipv6_with_separate_botport(
    tcl_bridge: BridgeClient, ipv6_required: None
) -> None:
    assert (
        tcl_bridge.eval_ok(f"addbot {BOT} \\[fe80::69ec:cfe4:81de:4fe5\\] 4444")
        == "1"
    )
    assert (
        tcl_bridge.eval_ok(f"getuser {BOT} botaddr")
        == "fe80::69ec:cfe4:81de:4fe5 4444 4444"
    )


def test_addbot_bracketed_ipv6_with_separate_botport_userport(
    tcl_bridge: BridgeClient, ipv6_required: None
) -> None:
    assert (
        tcl_bridge.eval_ok(
            f"addbot {BOT} \\[fe80::69ec:cfe4:81de:4fe5\\] 4444 5555"
        )
        == "1"
    )
    assert (
        tcl_bridge.eval_ok(f"getuser {BOT} botaddr")
        == "fe80::69ec:cfe4:81de:4fe5 4444 5555"
    )


def test_addbot_ipv6_with_trailing_slash_uses_default_ports(
    tcl_bridge: BridgeClient, ipv6_required: None
) -> None:
    """A trailing `/` after IPv6 means "no port given", default-port applies."""
    assert tcl_bridge.eval_ok(f"addbot {BOT} fe80::69ec:cfe4:81de:4fe5/") == "1"
    assert (
        tcl_bridge.eval_ok(f"getuser {BOT} botaddr")
        == f"fe80::69ec:cfe4:81de:4fe5 {DEFAULT_PORT} {DEFAULT_PORT}"
    )
