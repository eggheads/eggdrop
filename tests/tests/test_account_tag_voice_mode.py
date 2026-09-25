"""Account-tag CAP + WHOX + tagged MODE: regression for the full account-aware
path through CAP negotiation, WHOX-driven account learning, and an IRCv3-tagged
mode change.

End-to-end shape:
  - CAP LS advertises `account-tag`; bot REQ/server ACK; bot enables it.
  - 005 carries `WHOX` so eggdrop sets `use_354` (chan.c:3053) and emits
    `WHO #chan c%chnufat,222` after JOIN; the helper replies with 354s
    carrying op's account name "op".
  - The IRCd then sends a MODE +vv with an `@account=op` IRCv3 tag. Eggdrop
    parses the MODE (via standard mode handling) and the tag (via
    chan.c:gotrawt → setaccount).
  - Result: test1 and test2 are voiced; op's account is "op".
"""

from __future__ import annotations

import contextlib
from collections.abc import Iterator

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    wait_for_isupport,
)
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


@pytest.fixture
def mock_ircd() -> Iterator[MockIrcd]:
    """Override the default fixture to advertise `account-tag` in CAP LS.

    The cap list is fixed at construction time because the bot sends
    `CAP LS 302` immediately on TCP connect, before the test body runs.
    """
    ircd = MockIrcd(advertised_caps=["account-tag"]).start()
    try:
        yield ircd
    finally:
        with contextlib.suppress(Exception):
            ircd.stop()


def test_isvoice_after_tagged_mode_with_whox_and_account_tag(
    eggdrop_config,
    request: pytest.FixtureRequest,
) -> None:
    # account-tag is disabled by default in server.mod (servmsg.c:44); the
    # bot only adds it to the CAP REQ list when the Tcl var is 1. Set it via
    # extra_tcl so the assignment runs after server.mod's loadmodule.
    eggdrop_config.render(extra_tcl="set account-tag 1\n")
    mock_ircd: MockIrcd = request.getfixturevalue("mock_ircd")
    eggdrop_proc: EggdropProc = request.getfixturevalue("eggdrop_proc")  # noqa: F841 — proc must spawn before bridge
    tcl_bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    drive_registration(mock_ircd, isupport_tokens=["WHOX"])
    wait_for_isupport(tcl_bridge, "WHOX", "")  # bare token: value stored as ""

    # CAP negotiation must have completed (welcome only fires after CAP END);
    # account-tag must be in the enabled list.
    enabled = tcl_bridge.eval_ok("cap enabled").split()
    assert "account-tag" in enabled, f"expected account-tag enabled, got {enabled}"

    chan = drive_join_with_names(
        mock_ircd,
        "@op test1 test2 @TestBot",
        member_accounts={"op": "op"},
    )

    # WHOX populated op's account via 354 (chan.c:got354 → got352or4 → setaccount).
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getaccount op "{chan}"') == "op",
        timeout=5.0,
        description="WHOX 354 to set op's account to 'op'",
    )

    # Pre-MODE sanity: nobody has voice yet.
    assert tcl_bridge.eval_ok(f'onchan test1 "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'onchan test2 "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isvoice test1 "{chan}"') == "0"
    assert tcl_bridge.eval_ok(f'isvoice test2 "{chan}"') == "0"

    mock_ircd.send(f"@account=op :op!op@127.0.0.1 MODE {chan} +vv test1 :test2")

    wait_for(
        lambda: (
            tcl_bridge.eval_ok(f'isvoice test1 "{chan}"') == "1"
            and tcl_bridge.eval_ok(f'isvoice test2 "{chan}"') == "1"
        ),
        timeout=5.0,
        description="MODE +vv test1 test2 to voice both members",
    )

    # And op's account survived the tag-driven setaccount call (no-op since
    # it was already "op", but we want to be sure gotrawt didn't clobber it).
    assert tcl_bridge.eval_ok(f'getaccount op "{chan}"') == "op"
