"""Partyline integration test: drive HQ partyline via stdin, verify via bridge."""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


@pytest.mark.partyline
def test_partyline_add_channel(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Add a channel via the HQ partyline `.+chan` command, verify via bridge."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

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
        ) == "1",
        timeout=5.0,
        description="partyline .+chan #pytest to register",
    )
