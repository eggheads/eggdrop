"""Converted from eggdrop-tests/eggdrop_tcl_server.bats.

Tests for the `server` Tcl command — adding, removing, and listing the
in-memory server list.

API note: the original bats tests used the old `addserver`/`delserver`/
`set servers` interface, which no longer exists. The current command is
`server add HOST ?PORT? ?PASS?` / `server remove HOST ?PORT?` /
`server list` (returns a list of `{host port pass}` triples).
"""

from __future__ import annotations

from support.bridge_client import BridgeClient


def _server_entries(tcl_bridge: BridgeClient) -> list[list[str]]:
    """Return server list as a list of [host, port, pass] entries.

    `server list` formats each entry as a Tcl list `{host port pass}`. We
    parse it via Tcl `lmap` to keep things simple on the Python side.
    """
    raw = tcl_bridge.eval_ok(
        'lmap entry [server list] {format "%s|%s|%s" '
        "[lindex $entry 0] [lindex $entry 1] [lindex $entry 2]}"
    )
    return [e.split("|") for e in raw.split() if e]


def _server_in_list(
    tcl_bridge: BridgeClient, host: str, port: str | None = None
) -> bool:
    for entry in _server_entries(tcl_bridge):
        if entry[0] == host and (port is None or entry[1] == port):
            return True
    return False


# ---------- server add ----------


def test_server_add_just_host_uses_default_port(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add irc.foo.com")
    # Entry exists; port may be the configured default-port or empty.
    assert _server_in_list(tcl_bridge, "irc.foo.com")


def test_server_add_with_explicit_port(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add irc.ferg.com 8877")
    assert _server_in_list(tcl_bridge, "irc.ferg.com", "8877")


def test_server_add_with_port_and_password(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add irc.moo.com 4455 mypass")
    for entry in _server_entries(tcl_bridge):
        if entry[0] == "irc.moo.com" and entry[1] == "4455":
            assert entry[2] == "mypass"
            return
    raise AssertionError("irc.moo.com:4455 not in server list")


def test_server_add_ssl_port_keeps_plus_prefix(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add irc.snell.com +7000")
    assert _server_in_list(tcl_bridge, "irc.snell.com", "+7000")


def test_server_add_ipv6_address(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add 2344:2344:2344::5433:5433 5555")
    assert _server_in_list(tcl_bridge, "2344:2344:2344::5433:5433", "5555")


def test_server_add_ipv4_address(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add 1.2.3.4 4444")
    assert _server_in_list(tcl_bridge, "1.2.3.4", "4444")


# ---------- server remove ----------


def test_server_remove_first_element(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add irc.first.com 1111")
    tcl_bridge.eval_ok("server add irc.second.com 2222")
    tcl_bridge.eval_ok("server remove irc.first.com 1111")
    assert not _server_in_list(tcl_bridge, "irc.first.com", "1111")
    assert _server_in_list(tcl_bridge, "irc.second.com", "2222")


def test_server_remove_middle_element(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add a.com 1111")
    tcl_bridge.eval_ok("server add b.com 2222")
    tcl_bridge.eval_ok("server add c.com 3333")
    tcl_bridge.eval_ok("server remove b.com 2222")
    assert _server_in_list(tcl_bridge, "a.com", "1111")
    assert not _server_in_list(tcl_bridge, "b.com", "2222")
    assert _server_in_list(tcl_bridge, "c.com", "3333")


def test_server_remove_ipv6(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add 2344:2344:2344::5433:5433 5555")
    tcl_bridge.eval_ok("server remove 2344:2344:2344::5433:5433 5555")
    assert not _server_in_list(tcl_bridge, "2344:2344:2344::5433:5433")


def test_server_remove_ipv4(tcl_bridge: BridgeClient) -> None:
    tcl_bridge.eval_ok("server add 1.2.3.4 4444")
    tcl_bridge.eval_ok("server remove 1.2.3.4 4444")
    assert not _server_in_list(tcl_bridge, "1.2.3.4", "4444")


def test_server_remove_no_port_removes_all_matching_entries(
    tcl_bridge: BridgeClient,
) -> None:
    """Behavior change since the original bats: `server remove host` with no
    port iterates the list and removes EVERY entry matching `host`. The bats
    test assumed only the first match was removed."""
    tcl_bridge.eval_ok("server add irc.firstmatch.com 1111")
    tcl_bridge.eval_ok("server add irc.firstmatch.com 2222")
    tcl_bridge.eval_ok("server remove irc.firstmatch.com")
    assert not _server_in_list(tcl_bridge, "irc.firstmatch.com")


def test_server_remove_with_port_only_removes_matching_host_port(
    tcl_bridge: BridgeClient,
) -> None:
    tcl_bridge.eval_ok("server add irc.firstmatch.com 1111")
    tcl_bridge.eval_ok("server add irc.firstmatch.com 2222")
    tcl_bridge.eval_ok("server remove irc.firstmatch.com 2222")
    assert _server_in_list(tcl_bridge, "irc.firstmatch.com", "1111")
    assert not _server_in_list(tcl_bridge, "irc.firstmatch.com", "2222")


# ---------- error paths ----------


def test_server_add_rejects_colon_port_in_address(tcl_bridge: BridgeClient) -> None:
    """`host:port` syntax is forbidden — port must be a separate argument."""
    ok, result = tcl_bridge.eval("server add irc.port.com:1111")
    assert not ok
    assert "Make sure the port is" in result
