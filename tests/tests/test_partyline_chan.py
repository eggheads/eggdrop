"""Partyline integration test: drive the HQ partyline via stdin, verify via bridge."""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


def _complete_registration(mock_ircd: MockIrcd) -> None:
    mock_ircd.wait_for_connect(timeout=10.0)
    for _ in range(2):  # NICK + USER
        mock_ircd.recv(timeout=5.0)
    mock_ircd.send_welcome(nick="TestBot")
    mock_ircd.drain_until(lambda line: line.startswith("JOIN "), timeout=10.0)
    mock_ircd.send(":mock.test 353 TestBot = #test :@TestBot")
    mock_ircd.send(":mock.test 366 TestBot #test :End of /NAMES")


@pytest.mark.partyline
def test_partyline_add_channel(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Add a channel via the HQ partyline `.+chan` command, verify via bridge."""
    _complete_registration(mock_ircd)

    # Sanity: only the templated #test is configured.
    assert tcl_bridge.eval_ok("llength [channels]") == "1"

    # Drive the HQ partyline. The HQ user is `-HQ` with full owner perms in
    # -nt mode, so no auth handshake is needed.
    eggdrop_proc.send_partyline(".+chan #pytest")

    # The command runs asynchronously inside Eggdrop's event loop. Poll the
    # bridge until the new channel is visible (or the wait_for times out).
    wait_for(
        lambda: tcl_bridge.eval_ok(
            'expr {[lsearch [channels] "#pytest"] >= 0}'
        )
        == "1",
        timeout=5.0,
        description="partyline .+chan #pytest to register",
    )
