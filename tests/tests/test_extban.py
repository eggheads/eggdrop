"""Integration tests for the extban support PR (channels.mod / irc.mod).

Each test sets up only what it needs and reads from the bridge to assert on
internal state. Where IRC traffic is involved, the test drives the mock
IRCd directly and (when the bot's outgoing MODE is the thing under test)
flushes the mode buffer with `flushmode` to avoid waiting on the periodic
HOOK_IDLE flush.

What this PR introduced (and these tests cover):

- Tcl `account-extban` global, populated from ISUPPORT ACCOUNTEXTBAN.
- `.+extban <flag> <value>` partyline command. Constructs the mask using
  the EXTBAN-advertised prefix, refuses while disconnected.
- `u_addban` no longer auto-sticky-fies extban masks at storage time
  (option-1 swap: enforceability decided at runtime via
  `extban_is_unenforceable`, never persisted).
- `check_this_ban` / `recheck_bans` / `check_expired_chanstuff` treat
  unenforceable extbans (q:, c:, ...) as if sticky for set-and-keep
  purposes, even on `+dynamicbans` channels.
- `u_addban` skips the bot-self-ban check on extban masks (they don't
  have the nick!user@host shape that match would target).
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    wait_for_isupport,
)
from support.mock_ircd import MockIrcd
from support.userfile_helpers import format_userfile_ban
from support.waiters import wait_for

# ---------- Tcl `account-extban` global ----------


def test_account_extban_tcl_var_empty_when_no_isupport(
    tcl_bridge: BridgeClient,
) -> None:
    """Reading `$account-extban` before any 005 returns the empty string.

    The Tcl trace fires on read, calls `servermod_isupport_get("ACCOUNTEXTBAN")`,
    which returns NULL since the bot hasn't connected yet, and the trace
    handler stores "" in the variable.
    """
    assert tcl_bridge.eval_ok("set ::account-extban") == ""


def test_account_extban_tcl_var_populated_from_isupport(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """After 005 with `ACCOUNTEXTBAN=a`, reading the Tcl global returns "a".

    The PR also accepts the longer `a,account` form per ircdocs; we send
    the short form here because it's what most networks advertise.
    """
    drive_registration(
        mock_ircd,
        isupport_tokens=["EXTBAN=~,acrjmU", "ACCOUNTEXTBAN=a"],
    )
    wait_for_isupport(tcl_bridge, "ACCOUNTEXTBAN", "a")
    assert tcl_bridge.eval_ok("set ::account-extban") == "a"


# ---------- u_addban: no auto-sticky on extban storage ----------


def test_newban_extban_does_not_auto_sticky(
    tcl_bridge: BridgeClient,
) -> None:
    """`newban a:foo` stores the ban without `MASKREC_STICKY`.

    Before commit 9269ae64, u_addban set MASKREC_STICKY for any extban
    whose flag wasn't in the hardcoded enforceable set. With the
    option-1 swap, that decision moved to enforcement time
    (`extban_is_unenforceable`), so the userfile flags now reflect the
    user's literal intent.
    """
    tcl_bridge.eval_ok("newban a:foo testuser comment")
    assert tcl_bridge.eval_ok("isbansticky a:foo") == "0"
    assert tcl_bridge.eval_ok("isban a:foo") == "1"


def test_newban_extban_with_explicit_sticky_option_is_sticky(
    tcl_bridge: BridgeClient,
) -> None:
    """User-controlled sticky still works: `newban a:foo ... sticky` → sticky."""
    tcl_bridge.eval_ok("newban a:foo testuser comment 0 sticky")
    assert tcl_bridge.eval_ok("isbansticky a:foo") == "1"


def test_newban_unenforceable_extban_q_not_auto_sticky(
    tcl_bridge: BridgeClient,
) -> None:
    """`q:` (mute extban) is unenforceable from the eggdrop side, but the
    sticky bit still isn't persisted. Enforceability is a runtime decision."""
    tcl_bridge.eval_ok("newban q:badactor testuser comment")
    assert tcl_bridge.eval_ok("isbansticky q:badactor") == "0"
    assert tcl_bridge.eval_ok("isban q:badactor") == "1"


# ---------- u_addban: bot-self-ban check skipped for extbans ----------


def test_newban_extban_matching_botnick_is_not_self_rejected(
    tcl_bridge: BridgeClient,
) -> None:
    """The "I'm not going to ban myself" guard in u_addban only fires on
    the non-extban branch (extban masks don't have nick!user@host shape).
    `a:TestBot` should be accepted even though the bot's nick is TestBot.
    """
    tcl_bridge.eval_ok("newban a:TestBot testuser comment")
    assert tcl_bridge.eval_ok("isban a:TestBot") == "1"


# ---------- partyline `.+extban` ----------


@pytest.mark.partyline
def test_partyline_pls_extban_refused_when_disconnected(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`.+extban` requires ISUPPORT EXTBAN to know the prefix; refuses if
    we haven't seen 005 yet (commit 2e195b1).

    No drive_registration() call here — the bot has TCP-connected but is
    still pre-welcome, so isupport_get("EXTBAN") returns NULL.
    """
    snapshot = len(eggdrop_proc.stdout_text())
    eggdrop_proc.send_partyline(".+extban a foo")

    wait_for(
        lambda: "must be connected to a server with EXTBAN support"
        in eggdrop_proc.stdout_text()[snapshot:],
        timeout=5.0,
        description="partyline .+extban to refuse with EXTBAN-unavailable msg",
    )

    # And nothing was stored.
    assert tcl_bridge.eval_ok("isban a:foo") == "0"


@pytest.mark.partyline
def test_partyline_pls_extban_constructs_prefixed_mask(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`.+extban a Foo` on a server with `EXTBAN=~,a` stores `~a:Foo` in
    the channel ban list (prefix from ISUPPORT, flag and value from input).
    """
    drive_registration(
        mock_ircd,
        isupport_tokens=["EXTBAN=~,acrjmU", "ACCOUNTEXTBAN=a"],
    )
    drive_join_with_names(mock_ircd, "@TestBot")
    wait_for_isupport(tcl_bridge, "EXTBAN", "~,acrjmU")

    eggdrop_proc.send_partyline(".+extban a Foo #test")

    wait_for(
        lambda: tcl_bridge.eval_ok("isban ~a:Foo #test") == "1",
        timeout=5.0,
        description="partyline .+extban to register ~a:Foo on #test",
    )


@pytest.mark.partyline
def test_partyline_pls_extban_constructs_unprefixed_mask(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`.+extban a Foo` on a server whose EXTBAN advertises no prefix
    (form: `,types`) stores `a:Foo` (no prefix prepended).
    """
    drive_registration(mock_ircd, isupport_tokens=["EXTBAN=,acrjmU"])
    drive_join_with_names(mock_ircd, "@TestBot")
    wait_for_isupport(tcl_bridge, "EXTBAN", ",acrjmU")

    eggdrop_proc.send_partyline(".+extban a Foo #test")

    wait_for(
        lambda: tcl_bridge.eval_ok("isban a:Foo #test") == "1",
        timeout=5.0,
        description="partyline .+extban to register a:Foo (no prefix) on #test",
    )


# ---------- check_this_ban / recheck_bans: unenforceable extban set on +dynamicbans ----------


def test_unenforceable_extban_queued_as_plus_b_on_dynamicbans_channel(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A non-enforceable extban (`q:`) must still be set on the channel
    even with `+dynamicbans`, because eggdrop has no other way to enforce
    a server-side mute. The condition in check_this_ban includes
    `extban_is_unenforceable(banmask)` for exactly this case.
    """
    # 'q' must appear in the EXTBAN types list, otherwise check_this_ban
    # short-circuits at `!extban_flag_supported('q')` before add_mode.
    extban = "~,acjmqrUz"
    drive_registration(mock_ircd, isupport_tokens=[f"EXTBAN={extban}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    wait_for_isupport(tcl_bridge, "EXTBAN", extban)

    # Confirm preconditions for the +b add_mode path:
    # - dynamicbans is on (otherwise the path under test never gates)
    # - bot is op (HALFOP_CANTDOMODE('b') would short-circuit otherwise)
    assert tcl_bridge.eval_ok(f'channel get "{chan}" dynamicbans') == "1"
    assert tcl_bridge.eval_ok(f'isop TestBot "{chan}"') == "1"

    tcl_bridge.eval_ok(f'newchanban "{chan}" q:badmouth testuser comment')
    # Force-flush the mode buffer instead of waiting on HOOK_IDLE.
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    # The bot should send a MODE for chan that includes our extban as the +b
    # arg. The mode letters can be batched with chanmode protection (e.g.
    # "+tnb") so we just look for the unambiguous mask payload on a MODE line.
    mock_ircd.drain_until(
        lambda line: line.startswith(f"MODE {chan} ") and "q:badmouth" in line,
        timeout=5.0,
    )


def test_enforceable_account_extban_not_queued_on_dynamicbans_channel_without_match(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """An *enforceable* account extban for an account no current member has
    must NOT be set proactively on a `+dynamicbans` channel. The condition
    `extban_is_unenforceable("a:nooneactual")` returns false (because acc
    flag matches), so it falls back to the standard dynamic-bans rule:
    only set when a matching member triggers it.
    """
    drive_registration(
        mock_ircd,
        isupport_tokens=["EXTBAN=~,acrjmU", "ACCOUNTEXTBAN=a"],
    )
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    wait_for_isupport(tcl_bridge, "ACCOUNTEXTBAN", "a")
    assert tcl_bridge.eval_ok(f'channel get "{chan}" dynamicbans') == "1"

    tcl_bridge.eval_ok(f'newchanban "{chan}" a:nooneactual testuser comment')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    # No MODE +b should reach the IRCd within a reasonable window.
    # Use a short drain that *requires* a +b ... a:nooneactual to assert non-presence:
    # if the predicate never matches and we get a MockIrcdError on timeout, we win.
    from support.mock_ircd import MockIrcdError

    with pytest.raises(MockIrcdError):
        mock_ircd.drain_until(
            lambda line: line.startswith(f"MODE {chan} ") and "a:nooneactual" in line,
            timeout=2.0,
        )

    # And the userfile record exists regardless.
    assert tcl_bridge.eval_ok(f'isban a:nooneactual "{chan}"') == "1"
    # And it was not auto-stickified.
    assert tcl_bridge.eval_ok(f'isbansticky a:nooneactual "{chan}"') == "0"


def test_enforcebans_account_extban_kicks_after_account_change(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """+enforcebans + an account-extban + a member who was on the channel
    *before* the matching account was set: when the server sends ACCOUNT
    (account-notify capability), the bot must add +b for the extban and
    KICK the now-matching user.

    Path under test:
      ACCOUNT msg → got_account (chan.c:2877) → setaccount (chan.c:179) →
      banmask_list_matches_member finds the userfile ban → refresh_ban_kick
      → do_mask sets +b a:badname and kick_all sends KICK.

    `+enforcebans` is set explicitly so the test pins the "kick on
    account-match" intent independent of dynamic-bans defaults. Before
    the ACCOUNT message arrives, alice's m->account is empty, so
    `banmask_matches_member` correctly returns false and nothing fires.
    """
    drive_registration(
        mock_ircd,
        isupport_tokens=["EXTBAN=~,acrjmU", "ACCOUNTEXTBAN=a"],
    )
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    wait_for_isupport(tcl_bridge, "ACCOUNTEXTBAN", "a")

    # Preconditions for the kick path: bot is op, alice is on chan,
    # alice has no account yet, +enforcebans is on.
    assert tcl_bridge.eval_ok(f'isop TestBot "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1"
    tcl_bridge.eval_ok(f'channel set "{chan}" +enforcebans')
    assert tcl_bridge.eval_ok(f'channel get "{chan}" enforcebans') == "1"

    # Add the account-extban targeting an account alice doesn't yet have.
    # check_this_ban iterates members; alice's m->account is "" so
    # banmask_matches_member returns false. Nothing about a:badname goes
    # out — verified below by the negative drain.
    tcl_bridge.eval_ok(f'newchanban "{chan}" a:badname testuser comment')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    from support.mock_ircd import MockIrcdError

    with pytest.raises(MockIrcdError):
        mock_ircd.drain_until(
            lambda line: "a:badname" in line or (
                line.startswith(f"KICK {chan} alice")
            ),
            timeout=2.0,
        )

    # Alice logs in to the matching account.
    mock_ircd.send(":alice!u@h.example.com ACCOUNT badname")

    # Bot must (a) set +b a:badname on the channel and (b) KICK alice.
    # Modes get flushed by add_mode/flush_mode in the refresh_ban_kick
    # path; KICK goes via DP_SERVER directly. Both should arrive within
    # the IDLE flush cycle.
    seen = mock_ircd.drain_until(
        lambda line: line.startswith(f"KICK {chan} alice"),
        timeout=5.0,
    )
    assert any(
        line.startswith(f"MODE {chan} ") and "a:badname" in line
        for line in seen
    ), f"expected MODE +b a:badname before the KICK; got {seen}"


# ---------- userfile loading: extbans loaded regardless of EXTBAN advertised ----------


def test_extbans_load_from_userfile_before_connect(
    eggdrop_config,
    request: pytest.FixtureRequest,
) -> None:
    """Extbans stored in the userfile must be loaded into memory at startup,
    independent of any EXTBAN/ACCOUNTEXTBAN ISUPPORT data — that data isn't
    available until after the bot connects.

    The load path goes through `restore_chanban → addmask_fully` (users.c),
    which doesn't call `isupport_get` or any of the new extban helpers.
    Verified by spawning without driving registration. The bans are
    rendered into the userfile via the `userfile_bans` /
    `userfile_chan_bans` template variables (see templates/userfile.j2).
    """
    eggdrop_config.render(
        userfile_ban_lines=[
            format_userfile_ban(
                mask="a:storedacct", perm=True, sticky=False, expire=0,
                added=1700000000, lastactive=0, creator="owner",
                desc="loaded from userfile",
            ),
            format_userfile_ban(
                mask="q:storedmute", perm=True, sticky=False, expire=0,
                added=1700000000, lastactive=0, creator="owner",
                desc="loaded from userfile",
            ),
            format_userfile_ban(
                mask="U:strangers!*@*", perm=True, sticky=False, expire=0,
                added=1700000000, lastactive=0, creator="owner",
                desc="loaded from userfile",
            ),
        ],
        userfile_chan_ban_lines={
            "#test": [
                format_userfile_ban(
                    mask="~a:chanonlyacct", perm=True, sticky=False, expire=0,
                    added=1700000000, lastactive=0, creator="owner",
                    desc="loaded from userfile",
                ),
            ],
        },
    )
    request.getfixturevalue("eggdrop_proc")
    bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    # Bot has not yet connected — confirm ISUPPORT is empty for both keys
    # the new helpers consult. Then bans must still be present in memory.
    assert bridge.eval_ok("set ::account-extban") == ""

    # Global extbans loaded.
    assert bridge.eval_ok("isban a:storedacct") == "1"
    assert bridge.eval_ok("isban q:storedmute") == "1"
    assert bridge.eval_ok("isban U:strangers!*@*") == "1"

    # Per-channel extban loaded.
    assert bridge.eval_ok("isban ~a:chanonlyacct #test") == "1"

    # And — critically — none of them got auto-stickified by the new
    # u_addban code path, because the load path bypasses u_addban entirely.
    assert bridge.eval_ok("isbansticky a:storedacct") == "0"
    assert bridge.eval_ok("isbansticky q:storedmute") == "0"
    assert bridge.eval_ok("isbansticky ~a:chanonlyacct #test") == "0"


def test_extban_perm_sticky_flags_survive_userfile_roundtrip(
    eggdrop_config,
    request: pytest.FixtureRequest,
) -> None:
    """A perm + sticky extban *that was already in the userfile* loads with
    those flags intact. Verifies that the `+`/`*` flag characters in the
    record format aren't confused by the hex-escaped `:` in the mask.
    """
    eggdrop_config.render(
        userfile_ban_lines=[
            format_userfile_ban(
                mask="a:permsticky",
                perm=True,
                sticky=True,
                expire=0,
                added=1700000000,
                lastactive=0,
                creator="owner",
                desc="perm-sticky from userfile",
            ),
        ],
    )
    request.getfixturevalue("eggdrop_proc")
    bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    assert bridge.eval_ok("isban a:permsticky") == "1"
    assert bridge.eval_ok("isbansticky a:permsticky") == "1"
    assert bridge.eval_ok("ispermban a:permsticky") == "1"


# ---------- partyline .+ban / .+extban across connection states ----------


@pytest.mark.partyline
def test_partyline_pls_ban_extban_works_when_connected(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`.+ban a:foo` (extban via the generic +ban command, not +extban) is
    accepted while connected. The mask is stored verbatim — no prefix
    construction (that's +extban's job).
    """
    drive_registration(
        mock_ircd,
        isupport_tokens=["EXTBAN=~,acrjmU", "ACCOUNTEXTBAN=a"],
    )
    drive_join_with_names(mock_ircd, "@TestBot")
    wait_for_isupport(tcl_bridge, "EXTBAN", "~,acrjmU")

    eggdrop_proc.send_partyline(".+ban a:connectedacct #test why")

    wait_for(
        lambda: tcl_bridge.eval_ok("isban a:connectedacct #test") == "1",
        timeout=5.0,
        description="partyline .+ban (extban form) to register on #test",
    )
    # No auto-sticky.
    assert tcl_bridge.eval_ok("isbansticky a:connectedacct #test") == "0"


@pytest.mark.partyline
def test_partyline_pls_ban_extban_works_when_not_yet_connected(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`.+ban a:foo` works even before 005 has been received — `.+ban`
    has no connection gate (only `.+extban` does, since only `+extban`
    needs the prefix). The "extban not enabled" warning fires (because
    EXTBAN ISUPPORT is unknown) but the ban is still stored.
    """
    # Deliberately: NO drive_registration() — bot is pre-welcome.
    snapshot = len(eggdrop_proc.stdout_text())
    eggdrop_proc.send_partyline(".+ban a:disconnectedacct why")

    wait_for(
        lambda: tcl_bridge.eval_ok("isban a:disconnectedacct") == "1",
        timeout=5.0,
        description="partyline .+ban (extban form) to register while disconnected",
    )

    # Storage: not auto-stickified (regression for option-1 swap).
    assert tcl_bridge.eval_ok("isbansticky a:disconnectedacct") == "0"

    # And the user got the EXTBAN-not-enabled feedback. The message text
    # comes from EXTBAN_NOT_ENABLED1/2/3 in the language file; we look for
    # a stable substring rather than the full template.
    new_output = eggdrop_proc.stdout_text()[snapshot:]
    assert "extban is not enabled on this server" in new_output, new_output


@pytest.mark.partyline
def test_partyline_pls_extban_works_when_connected_already_covered(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Sanity counterpart to the `.+extban` disconnected test above:
    same command, but with 005 received first, succeeds and stores the
    prefixed mask. (Mostly redundant with `_constructs_prefixed_mask`
    above — included so the connected/disconnected pair reads cleanly
    next to each other.)
    """
    drive_registration(mock_ircd, isupport_tokens=["EXTBAN=~,acrjmU"])
    drive_join_with_names(mock_ircd, "@TestBot")
    wait_for_isupport(tcl_bridge, "EXTBAN", "~,acrjmU")

    eggdrop_proc.send_partyline(".+extban a Bar #test")

    wait_for(
        lambda: tcl_bridge.eval_ok("isban ~a:Bar #test") == "1",
        timeout=5.0,
        description="connected .+extban to register ~a:Bar on #test",
    )


# ---------- partyline behaviour without server.mod loaded ----------


@pytest.mark.partyline
def test_partyline_pls_ban_extban_works_without_server_mod(
    eggdrop_config,
    request: pytest.FixtureRequest,
) -> None:
    """`.+ban a:foo` stores the extban even when server.mod isn't loaded
    at all — the storage path doesn't need ISUPPORT, and the
    `servermod_isupport_get` thunk returns NULL safely when
    `module_find("server")` finds nothing.

    Without server.mod: irc.mod and ctcp.mod also can't load (they
    `module_depend` on server). channels.mod loads cleanly because the
    cross-module API bypass was fixed (commit 8cfcd51e). The bot has no
    outbound IRC connection at all in this configuration.

    Driven via the `modules` template variable — see
    tests/templates/eggdrop.conf.j2 and the `EggdropConfig.context()`
    default in conftest.py. Render must happen *before* the proc fixture
    evaluates, so the proc/bridge are pulled in lazily via getfixturevalue.
    """
    eggdrop_config.render(modules=["pbkdf2", "channels", "console", "notes"])
    proc: EggdropProc = request.getfixturevalue("eggdrop_proc")
    bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    # Sanity: server.mod is genuinely not loaded.
    assert bridge.eval_ok("expr {[catch {set ::server-online}] != 0}") == "1"

    # `.+ban` with an extban mask: stored, no errors.
    proc.send_partyline(".+ban a:noservermod why")
    wait_for(
        lambda: bridge.eval_ok("isban a:noservermod") == "1",
        timeout=5.0,
        description=".+ban (extban form) to register without server.mod",
    )
    assert bridge.eval_ok("isbansticky a:noservermod") == "0"

    # `.+extban`: refused. The thunk returns NULL → no EXTBAN known →
    # gated by the same check that fires when disconnected.
    snapshot = len(proc.stdout_text())
    proc.send_partyline(".+extban a foo")
    wait_for(
        lambda: "must be connected to a server with EXTBAN support"
        in proc.stdout_text()[snapshot:],
        timeout=5.0,
        description=".+extban refusal text without server.mod loaded",
    )
    # Nothing stored from the +extban attempt.
    assert bridge.eval_ok("isban a:foo") == "0"
    assert bridge.eval_ok("isban ~a:foo") == "0"
