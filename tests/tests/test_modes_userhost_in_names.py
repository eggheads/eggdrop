"""A4: the userhost-in-names NAMES path is status-blind (arbmodes3 step 0).

Separate module because the cap must be advertised in CAP LS at MockIrcd
construction time (CAP ACK only enables caps that have a record, and
records are created from the LS listing), which requires overriding the
`mock_ircd` fixture for the whole module.
"""

from __future__ import annotations

import contextlib
from collections.abc import Iterator

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


@pytest.fixture
def mock_ircd() -> Iterator[MockIrcd]:
    ircd = MockIrcd(advertised_caps=["userhost-in-names"]).start()
    try:
        yield ircd
    finally:
        with contextlib.suppress(Exception):
            ircd.stop()


def test_a4_userhost_in_names_creates_member_without_status(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """With the userhost-in-names cap enabled, a 353 populates the member
    and its userhost but deliberately discards prefix status (the 353
    handler passes empty WHO flags). Pins D-PFX5's 'NAMES logic unchanged'.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot plain")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan plain "{chan}"') == "1",
        timeout=5.0,
        description="plain to appear on the channel",
    )

    # Enable the cap post-registration; the mock ACKs any REQ and the
    # record exists because the cap was advertised in LS.
    tcl_bridge.eval_ok("cap req userhost-in-names")
    wait_for(
        lambda: "userhost-in-names" in tcl_bridge.eval_ok("cap enabled"),
        timeout=5.0,
        description="userhost-in-names cap to be enabled",
    )

    mock_ircd.send(f":mock.test 353 TestBot = {chan} :@newguy!nu@nh.example.com")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan newguy "{chan}"') == "1",
        timeout=5.0,
        description="newguy to appear via the NAMES path",
    )
    assert tcl_bridge.eval_ok(f'getchanhost newguy "{chan}"') == "nu@nh.example.com"
    # The @ prefix is stripped and lost: newguy is NOT op.
    assert tcl_bridge.eval_ok(f'isop newguy "{chan}"') == "0"
