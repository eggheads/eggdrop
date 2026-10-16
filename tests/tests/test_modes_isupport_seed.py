"""B1: ISUPPORT seeding + replay (arbmodes3 step 1).

server.mod parses the compiled ISUPPORT defaults eagerly in
`isupport_init()` and exports `isupport_replay()`; irc.mod calls it in
`irc_start` right after adding its isupport binds. Two properties follow,
both observed through the HQ partyline `.status all` report, which prints
irc.mod's live `modecharinfo` tables (`tell_modeparsing`):

1. `modecharinfo` is populated from the defaults *before any connect* --
   previously it stayed empty until the first 005.
2. A server's live 005 values survive an irc.mod unload/reload while
   connected -- the reloaded module starts with an empty table and replay
   re-pulls the values server.mod still holds. Without replay the table
   would fall back to compiled defaults until the next value change.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import drive_registration
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


def status_all_tail(eggdrop_proc: EggdropProc, timeout: float = 5.0) -> str:
    """Run `.status all` on the HQ partyline and return only the report
    text produced by this call (the stdout log is cumulative, so a naive
    search would also match a previous report)."""
    offset = len(eggdrop_proc.stdout_text())
    eggdrop_proc.send_partyline(".status all")

    def _have_report() -> str | None:
        tail = eggdrop_proc.stdout_text()[offset:]
        # The Prefix line is emitted last in tell_modeparsing(), so its
        # presence means the whole mode-parsing block has been printed.
        return tail if "Prefix modes:" in tail else None

    wait_for(
        lambda: _have_report() is not None,
        timeout=timeout,
        description=".status all mode-parsing report",
    )
    return _have_report() or ""


@pytest.mark.partyline
def test_b1_defaults_seeded_before_connect(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """No registration is driven: the bot is up but never connected. The
    default PREFIX=(ohv)@%+ and CHANMODES list section must already be
    parsed into modecharinfo."""
    report = status_all_tail(eggdrop_proc)

    prefix_line = next(
        ln for ln in report.splitlines() if "Prefix modes:" in ln
    )
    assert "o(@)" in prefix_line, prefix_line
    assert "h(%)" in prefix_line, prefix_line
    assert "v(+)" in prefix_line, prefix_line

    list_line = next(ln for ln in report.splitlines() if "List modes:" in ln)
    for mode in ("b", "e", "I"):
        assert mode in list_line, (mode, list_line)


@pytest.mark.partyline
def test_b1_server_isupport_survives_irc_reload(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A non-default flag mode `Z` advertised in 005 is reflected after
    registration, and -- the point of replay -- still reflected after
    irc.mod is unloaded and loaded again while the connection (and thus
    server.mod's isupport records) stays up."""
    drive_registration(
        mock_ircd,
        isupport_tokens=["CHANMODES=beI,k,l,imnpstZ", "PREFIX=(ohv)@%+"],
    )

    def flag_line(report: str) -> str:
        return next(ln for ln in report.splitlines() if "Flag modes:" in ln)

    # Server value applied through the normal 005 path.
    wait_for(
        lambda: "Z" in flag_line(status_all_tail(eggdrop_proc)),
        timeout=5.0,
        description="server-advertised flag mode Z to be learned",
    )

    # Reload irc.mod. The .so is dlclose'd, so modecharinfo is re-zeroed;
    # server.mod (and its isupport records) stay loaded.
    eggdrop_proc.send_partyline(".unloadmod irc")
    eggdrop_proc.send_partyline(".loadmod irc")
    wait_for(
        lambda: tcl_bridge.eval_ok('expr {[lsearch -index 0 [modules] irc] >= 0}')
        == "1",
        timeout=5.0,
        description="irc.mod to be loaded again",
    )

    # Replay must have re-pulled the server's Z (compiled default has none).
    wait_for(
        lambda: "Z" in flag_line(status_all_tail(eggdrop_proc)),
        timeout=5.0,
        description="flag mode Z to survive the irc.mod reload",
    )
