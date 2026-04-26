"""End-to-end smoke test: spawn Eggdrop, register, join, introspect via bridge."""

from __future__ import annotations


def test_bridge_alive(tcl_bridge):
    """Eggdrop boots, the bridge listens, Tcl evaluates."""
    assert tcl_bridge.eval_ok("expr {2 + 2}") == "4"
    assert tcl_bridge.eval_ok("info patchlevel")  # non-empty
    assert tcl_bridge.eval_ok("set ::nick") == "TestBot"


def test_connect_register_join_and_introspect(eggdrop_proc, mock_ircd, tcl_bridge):
    """Full happy path: TCP connect, NICK/USER, welcome, JOIN, NAMES, introspect."""
    mock_ircd.wait_for_connect(timeout=10.0)

    # Eggdrop sends NICK and USER (order not guaranteed but typically NICK first).
    seen: set[str] = set()
    for _ in range(2):
        line = mock_ircd.recv(timeout=5.0)
        cmd = line.split(maxsplit=1)[0].upper()
        assert cmd in ("NICK", "USER"), f"unexpected line: {line!r}"
        seen.add(cmd)
    assert seen == {"NICK", "USER"}

    mock_ircd.send_welcome(nick="TestBot")

    # On 001 Eggdrop queues WHOIS (for its own user@host) and JOIN, dequeued
    # one per second. Skip past WHOIS to find the JOIN.
    join_line = mock_ircd.drain_until(
        lambda line: line.startswith("JOIN "), timeout=10.0
    )[-1]
    chan = join_line.split()[1]
    assert chan == "#test"

    # Acknowledge the join with NAMES so Eggdrop sees itself in the channel.
    mock_ircd.send(f":mock.test 353 TestBot = {chan} :@TestBot")
    mock_ircd.send(f":mock.test 366 TestBot {chan} :End of /NAMES")

    # Eggdrop is configured with #test. Use lsearch in Tcl so list quoting
    # (curly-brace wrapping of names that start with '#') doesn't trip us up.
    assert tcl_bridge.eval_ok('expr {[lsearch [channels] "#test"] >= 0}') == "1"

    # Owner host from the rendered userfile is *!*@127.0.0.1.
    hosts = tcl_bridge.eval_ok("getuser owner HOSTS")
    assert "127.0.0.1" in hosts
