"""Integration tests for the extban support PR (channels.mod / irc.mod).

Each test sets up only what it needs and reads from the bridge to assert on
internal state. Where IRC traffic is involved, the test drives the mock
IRCd directly and (when the bot's outgoing MODE is the thing under test)
flushes the mode buffer with `flushmode` to avoid waiting on the periodic
HOOK_IDLE flush.

Behaviour under test, per doc/sphinx_source/using/bans.rst ("Extended Bans")
and the extban normalization in commit 38614e90 (#1920), which replaced an
earlier design where enforceability was decided at runtime and never
persisted. Tests written against that earlier design were corrected here
and in #1924; the two arrived at the same conclusions independently.

The implementation in channels.mod / irc.mod:

- Tcl `account-extban` global, populated from ISUPPORT ACCOUNTEXTBAN.
- `.+extban <flag> <value>` partyline command. Constructs the mask using
  the EXTBAN-advertised prefix, refuses while disconnected.
- Three tiers of extban flag (channels.mod/channels.h):
    * ENFORCEABLE_EXTBANS "U", plus whatever ACCOUNTEXTBAN advertises.
      These can be enforced with a kick under +enforcebans.
    * MATCHABLE_EXTBANS "UABCmNpqQT". Eggdrop can match the argument
      against nick!user@host, so these behave like ordinary bans and
      obey +dynamicbans.
    * Everything else is unmatchable. `u_addban` (userchan.c) sets
      MASKREC_STICKY on these *at storage time*, so `check_this_ban`
      later sets and keeps them on the channel regardless of
      +dynamicbans -- Eggdrop has no other way to enforce them.
- Matchability is evaluated against ISUPPORT at the moment of storage.
  A flag is unmatchable while disconnected if it isn't in
  MATCHABLE_EXTBANS, because ACCOUNTEXTBAN isn't known yet.
- `u_addban` skips the bot-self-ban check on extban masks (they don't
  have the nick!user@host shape that match would target).

NB: "matchable" and "enforceable" are distinct predicates. `q` is
matchable but not kick-enforceable; do not conflate them.

Confirmed intent (maintainer, Aug 2026): a ban is classified against the
flags known at the moment it is set. If ACCOUNTEXTBAN is not known for
any reason -- disconnected, no server.mod, server does not advertise it --
then it is not a known flag, so the ban is stored sticky unless it
matches one of the static flags. The classification is deliberately not
re-evaluated later.
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
from support.mock_ircd import MockIrcd, MockIrcdError
from support.userfile_helpers import format_userfile_ban
from support.waiters import wait_for

# ---------- Tcl `account-extban` global ----------


def test_account_extban_tcl_var_empty_when_no_isupport(
    tcl_bridge: BridgeClient,
) -> None:
    """Before connecting to a server, the account extban flag exposed to
    scripts is empty because no server has advertised one.

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
    """Once a server advertises which extban flag means 'account', that flag
    is exposed to scripts.

    The PR also accepts the longer `a,account` form per ircdocs; we send
    the short form here because it's what most networks advertise.
    """
    drive_registration(
        mock_ircd,
        isupport_tokens=["EXTBAN=~,acrjmU", "ACCOUNTEXTBAN=a"],
    )
    wait_for_isupport(tcl_bridge, "ACCOUNTEXTBAN", "a")
    assert tcl_bridge.eval_ok("set ::account-extban") == "a"


# ---------- u_addban: unmatchable extbans are stored sticky ----------


def test_newban_account_extban_auto_sticky_while_disconnected(
    tcl_bridge: BridgeClient,
) -> None:
    """An extended ban that Eggdrop cannot evaluate itself is stored as
    sticky, so it will be set on the channel and kept there.

    Per bans.rst: "If the extban is a valid flag but cannot be matched by
    Eggdrop, it is automatically stored as sticky. Because it cannot be
    matched, it will be kept set on the channel despite any dynamic-ban
    channel status set."

    `a` is not in MATCHABLE_EXTBANS ("UABCmNpqQT"), and ACCOUNTEXTBAN is
    unknown before connect, so `extban_is_matchable` returns 0 and
    u_addban (userchan.c:478) ORs in MASKREC_STICKY.
    """
    tcl_bridge.eval_ok("newban a:foo testuser comment")
    assert tcl_bridge.eval_ok("isban a:foo") == "1"
    assert tcl_bridge.eval_ok("isbansticky a:foo") == "1"


def test_newban_account_extban_not_sticky_once_accountextban_known(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """The same extended ban stored while connected to a server that supports
    it is not made sticky, because Eggdrop can evaluate it.

    This is the companion to the test above and pins the fact that
    matchability is resolved against ISUPPORT at storage time.
    """
    drive_registration(
        mock_ircd,
        isupport_tokens=["EXTBAN=~,acrjmU", "ACCOUNTEXTBAN=a"],
    )
    wait_for_isupport(tcl_bridge, "ACCOUNTEXTBAN", "a")

    tcl_bridge.eval_ok("newban a:laterfoo testuser comment")
    assert tcl_bridge.eval_ok("isban a:laterfoo") == "1"
    assert tcl_bridge.eval_ok("isbansticky a:laterfoo") == "0"


def test_newban_unmatchable_extban_j_auto_sticky(
    tcl_bridge: BridgeClient,
) -> None:
    """An extban flag that is neither matchable nor the account letter is
    stored sticky whatever the connection state.

    `j:` is absent from MATCHABLE_EXTBANS and can never become the account
    letter, so unlike `a:` its classification does not depend on ISUPPORT.
    Sticky is the mechanism that keeps unmatchable extbans set on
    +dynamicbans channels (38614e90).
    """
    tcl_bridge.eval_ok("newban j:#badchan testuser comment")
    assert tcl_bridge.eval_ok("isbansticky j:#badchan") == "1"
    assert tcl_bridge.eval_ok("isban j:#badchan") == "1"


def test_newban_extban_with_explicit_sticky_option_is_sticky(
    tcl_bridge: BridgeClient,
) -> None:
    """Asking for an extended ban to be sticky still works and is honoured.
    """
    tcl_bridge.eval_ok("newban a:foo testuser comment 0 sticky")
    assert tcl_bridge.eval_ok("isbansticky a:foo") == "1"


def test_newban_matchable_extban_q_not_auto_sticky(
    tcl_bridge: BridgeClient,
) -> None:
    """An extended ban Eggdrop can evaluate is not made sticky, even though
    Eggdrop cannot kick anyone for violating it.

    This is the pair that distinguishes the two predicates: enforceable
    (ENFORCEABLE_EXTBANS "U" + ACCOUNTEXTBAN) governs kicks, matchable
    (MATCHABLE_EXTBANS) governs sticky/dynamic placement.
    """
    tcl_bridge.eval_ok("newban q:badactor testuser comment")
    assert tcl_bridge.eval_ok("isban q:badactor") == "1"
    assert tcl_bridge.eval_ok("isbansticky q:badactor") == "0"


# ---------- u_addban: bot-self-ban check skipped for extbans ----------


def test_newban_extban_matching_botnick_is_not_self_rejected(
    tcl_bridge: BridgeClient,
) -> None:
    """The safeguard that stops Eggdrop banning itself does not reject
    extended bans, whose format cannot describe the bot's own address.
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
    """The dedicated extban command is refused while disconnected, because the
    correct prefix is only known once a server advertises it.

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
    """The dedicated extban command builds the full mask by prepending the
    prefix the server advertised.
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
    """On a server that uses no extban prefix, the dedicated extban command
    builds the mask without one.
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


def test_unmatchable_extban_queued_as_plus_b_on_dynamicbans_channel(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """An extended ban Eggdrop cannot evaluate is placed on the channel and
    kept there even where bans are normally set only on demand.

    Mechanism: u_addban stickies it at storage time (not matchable), and
    check_this_ban re-reads that persisted sticky bit via u_sticky_mask
    before the `!channel_dynamicbans(chan) || sticky` gate.

    'j' must appear in the EXTBAN types list, otherwise check_this_ban
    short-circuits at `!extban_flag_supported('j')` before add_mode. It
    must NOT appear in MATCHABLE_EXTBANS ("UABCmNpqQT") -- it doesn't.

    This also covers the record-flag lookup in check_this_ban: Tcl
    newchanban passes sticky=0, so the push happens only because
    check_this_ban re-derives stickiness from the stored record.
    """
    extban = "~,acjmqrUz"
    drive_registration(mock_ircd, isupport_tokens=[f"EXTBAN={extban}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    wait_for_isupport(tcl_bridge, "EXTBAN", extban)

    # Confirm preconditions for the +b add_mode path:
    # - dynamicbans is on (otherwise the path under test never gates)
    # - bot is op (HALFOP_CANTDOMODE('b') would short-circuit otherwise)
    assert tcl_bridge.eval_ok(f'channel get "{chan}" dynamicbans') == "1"
    assert tcl_bridge.eval_ok(f'isop TestBot "{chan}"') == "1"

    tcl_bridge.eval_ok(f'newchanban "{chan}" j:#badchan testuser comment')
    # The sticky bit is what drives the set-and-keep behaviour; assert it
    # directly so a failure here distinguishes "not stickied" from
    # "stickied but not set".
    assert tcl_bridge.eval_ok(f'isbansticky j:#badchan "{chan}"') == "1"

    # Force-flush the mode buffer instead of waiting on HOOK_IDLE.
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    # The bot should send a MODE for chan that includes our extban as the +b
    # arg. The mode letters can be batched with chanmode protection (e.g.
    # "+tnb") so we just look for the unambiguous mask payload on a MODE line.
    mock_ircd.drain_until(
        lambda line: line.startswith(f"MODE {chan} ") and "j:#badchan" in line,
        timeout=5.0,
    )


def test_matchable_extban_not_queued_on_dynamicbans_channel_without_match(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """An extended ban Eggdrop can evaluate follows the usual on-demand rule
    and is not placed until somebody it matches is present.

    Because q is in MATCHABLE_EXTBANS, u_addban leaves it unsticky, so
    check_this_ban falls through to the ordinary dynamic-bans rule: set
    only when a member actually matches.
    """
    extban = "~,acjmqrUz"
    drive_registration(mock_ircd, isupport_tokens=[f"EXTBAN={extban}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    wait_for_isupport(tcl_bridge, "EXTBAN", extban)
    assert tcl_bridge.eval_ok(f'channel get "{chan}" dynamicbans') == "1"

    tcl_bridge.eval_ok(f'newchanban "{chan}" q:*!*@nobody.example testuser comment')
    assert tcl_bridge.eval_ok(f'isbansticky q:*!*@nobody.example "{chan}"') == "0"
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    with pytest.raises(MockIrcdError):
        mock_ircd.drain_until(
            lambda line: line.startswith(f"MODE {chan} ")
            and "nobody.example" in line,
            timeout=2.0,
        )


def test_enforceable_account_extban_not_queued_on_dynamicbans_channel_without_match(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """An account ban for an account nobody present is using is stored but not
    placed on the channel.
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
    """When a member logs in to an account that is banned, Eggdrop sets the
    ban and kicks them.

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
    """Extended bans saved in the user file are loaded at startup, before any
    server has said which kinds it supports.

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
    """A permanent, sticky extended ban keeps both of those attributes when
    reloaded from the user file.
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
    """The ordinary ban command accepts an extended ban mask and stores it
    exactly as typed.
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
    """The ordinary ban command accepts an extended ban mask before
    connecting, warning that support is unknown but still storing it.
    """
    # Deliberately: NO drive_registration() — bot is pre-welcome.
    snapshot = len(eggdrop_proc.stdout_text())
    eggdrop_proc.send_partyline(".+ban a:disconnectedacct why")

    wait_for(
        lambda: tcl_bridge.eval_ok("isban a:disconnectedacct") == "1",
        timeout=5.0,
        description="partyline .+ban (extban form) to register while disconnected",
    )

    # Stickied at storage time: while disconnected ACCOUNTEXTBAN is unknown
    # and 'a' is not in MATCHABLE_EXTBANS, so u_addban marks it sticky. Per
    # bans.rst, an extban Eggdrop cannot match is kept set on the channel.
    assert tcl_bridge.eval_ok("isbansticky a:disconnectedacct") == "1"

    # And the user got the EXTBAN-not-enabled feedback. The message text
    # comes from EXTBAN_NOT_ENABLED1/2/3 in the language file; we look for
    # a stable substring rather than the full template.
    new_output = eggdrop_proc.stdout_text()[snapshot:]
    assert "cannot be matched by Eggdrop" in new_output, new_output
    assert "extban is not enabled on this server" in new_output, new_output


@pytest.mark.partyline
def test_partyline_pls_extban_works_when_connected_already_covered(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """The dedicated extban command succeeds once connected to a server that
    advertises extban support.
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
    """Extended bans can be stored with no server module loaded at all, while
    the dedicated extban command is refused because no prefix is knowable.

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
    # No server.mod means no ISUPPORT at all, so the mask is unmatchable and
    # u_addban stickies it -- same rule as the disconnected case above.
    assert bridge.eval_ok("isbansticky a:noservermod") == "1"

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

# ---------- dynamic ban-time expiry for channel banlist masks ----------


@pytest.mark.timeout(90)
def test_dynamic_ban_time_expiry_removes_normal_and_extban_channel_modes(
    eggdrop_config,
    mock_ircd: MockIrcd,
    request: pytest.FixtureRequest,
) -> None:
    """Where bans expire automatically, both ordinary and extended channel
    bans are removed when their time is up, and neither is removed on
    channels where expiry is off.

    This drives server-side +b modes rather than internal userfile bans so the
    assertions cover the channel banlist cleanup path in irc.mod. `ban-time` is
    set negative to make the masks eligible on the next minutely hook without a
    multi-minute sleep; the dynamicbans gate is still the real one.
    """
    eggdrop_config.render(
        channels=[
            {"name": "#dyn", "chanmode": "+nt"},
            {"name": "#static", "chanmode": "+nt"},
        ]
    )
    request.getfixturevalue("eggdrop_proc")
    tcl_bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    drive_registration(
        mock_ircd,
        isupport_tokens=["EXTBAN=$,aUq", "ACCOUNTEXTBAN=a"],
    )
    dyn_chan = drive_join_with_names(mock_ircd, "@TestBot")
    static_chan = drive_join_with_names(mock_ircd, "@TestBot")
    assert dyn_chan == "#dyn"
    assert static_chan == "#static"

    tcl_bridge.eval_ok(f'channel set "{dyn_chan}" +dynamicbans ban-time -1')
    tcl_bridge.eval_ok(f'channel set "{static_chan}" -dynamicbans ban-time -1')
    assert tcl_bridge.eval_ok(f'channel get "{dyn_chan}" dynamicbans') == "1"
    assert tcl_bridge.eval_ok(f'channel get "{static_chan}" dynamicbans') == "0"

    dynamic_masks = ["*!*@dynamic.example", "$a:geo"]
    static_masks = ["*!*@static.example", "$a:staticgeo"]

    for mask in dynamic_masks:
        mock_ircd.send(f":Oper!oper@mock MODE {dyn_chan} +b {mask}")
    for mask in static_masks:
        mock_ircd.send(f":Oper!oper@mock MODE {static_chan} +b {mask}")

    for mask in dynamic_masks:
        wait_for(
            lambda mask=mask: tcl_bridge.eval_ok(
                f"ischanban {{{mask}}} {{{dyn_chan}}}"
            ) == "1",
            timeout=5.0,
            description=f"{mask} to appear on {dyn_chan}",
        )
    for mask in static_masks:
        wait_for(
            lambda mask=mask: tcl_bridge.eval_ok(
                f"ischanban {{{mask}}} {{{static_chan}}}"
            ) == "1",
            timeout=5.0,
            description=f"{mask} to appear on {static_chan}",
        )

    removed_dynamic: set[str] = set()

    def saw_dynamic_removal(line: str) -> bool:
        if line.startswith(f"MODE {dyn_chan} ") and "-b" in line:
            for mask in dynamic_masks:
                if mask in line:
                    removed_dynamic.add(mask)
        return len(removed_dynamic) == len(dynamic_masks)

    mock_ircd.drain_until(saw_dynamic_removal, timeout=70.0)
    assert removed_dynamic == set(dynamic_masks)

    with pytest.raises(MockIrcdError):
        mock_ircd.drain_until(
            lambda line: line.startswith(f"MODE {static_chan} ")
            and "-b" in line
            and any(mask in line for mask in static_masks),
            timeout=2.0,
        )


# ---------- documented claims that previously had no coverage ----------


def test_longform_extban_flag_is_not_treated_as_extban(
    tcl_bridge: BridgeClient,
) -> None:
    """Only single-letter extban flags are recognised; a spelled-out flag name
    is not treated as an extended ban.

    `extban_parse` requires `<alnum>:` or `<prefix><alnum>:`, so
    "account:foo" fails to parse as an extban and falls through to the
    ordinary mask path in u_addban -- which means fix_broken_mask()
    rewrites it into nick!user@host shape rather than storing it verbatim.

    This test pins that longform input does NOT round-trip as given, which
    is what a user typing `.+ban account:foo` would otherwise assume.
    """
    tcl_bridge.eval_ok("newban account:foo testuser comment")
    assert tcl_bridge.eval_ok("isban account:foo") == "0"


def test_unsupported_extban_flag_stored_but_not_set_on_channel(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """An extended ban using a flag the current server does not support is
    kept internally but never set on the channel.

    `z` is absent from the advertised EXTBAN types, so check_this_ban
    returns early at `!extban_flag_supported('z')` and never reaches
    add_mode -- even though the mask was stickied at storage time for
    being unmatchable.
    """
    extban = "~,acjmqrU"  # deliberately no 'z'
    drive_registration(mock_ircd, isupport_tokens=[f"EXTBAN={extban}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    wait_for_isupport(tcl_bridge, "EXTBAN", extban)

    tcl_bridge.eval_ok(f'newchanban "{chan}" z:unsupported testuser comment')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    # Stored internally...
    assert tcl_bridge.eval_ok(f'isban z:unsupported "{chan}"') == "1"

    # ...but never placed on the channel.
    with pytest.raises(MockIrcdError):
        mock_ircd.drain_until(
            lambda line: line.startswith(f"MODE {chan} ")
            and "z:unsupported" in line,
            timeout=2.0,
        )


def test_enforcebans_U_extban_kicks_matching_member(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """The unregistered-user extended ban is one Eggdrop can enforce itself,
    kicking a matching member.

    The ACCOUNTEXTBAN half is covered by
    test_enforcebans_account_extban_kicks_after_account_change; this is the
    `U` half, which nothing exercised. banmask_enforces_member matches the
    U argument against nick!user@host via match_addr.
    """
    drive_registration(mock_ircd, isupport_tokens=["EXTBAN=~,acrjmU"])
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    wait_for_isupport(tcl_bridge, "EXTBAN", "~,acrjmU")

    assert tcl_bridge.eval_ok(f'isop TestBot "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1"
    tcl_bridge.eval_ok(f'channel set "{chan}" +enforcebans')
    assert tcl_bridge.eval_ok(f'channel get "{chan}" enforcebans') == "1"

    tcl_bridge.eval_ok(f'newchanban "{chan}" U:*!*@h.example.com testuser comment')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    mock_ircd.drain_until(
        lambda line: line.startswith(f"KICK {chan} alice"),
        timeout=5.0,
    )


def test_sticky_decision_is_persisted_not_reevaluated(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A ban stored sticky before connecting stays sticky afterwards.

    Matchability is evaluated once, in u_addban, and the resulting
    MASKREC_STICKY bit is written to the userfile. Connecting to a server
    that advertises ACCOUNTEXTBAN=a does not revisit the decision for
    bans already stored.

    This is intended behaviour, not an oversight: if Eggdrop could not
    evaluate the ban at the time it was set, it keeps it set rather than
    risk silently dropping it. Pinned here because "the flag is supported
    now, why is it still sticky?" is an easy misreading.
    """
    # Stored while disconnected: ACCOUNTEXTBAN unknown -> sticky.
    tcl_bridge.eval_ok("newban a:earlyacct testuser comment")
    assert tcl_bridge.eval_ok("isbansticky a:earlyacct") == "1"

    # Now connect to a server that does support the flag.
    drive_registration(
        mock_ircd,
        isupport_tokens=["EXTBAN=~,acjmqrUz", "ACCOUNTEXTBAN=a"],
    )
    wait_for_isupport(tcl_bridge, "ACCOUNTEXTBAN", "a")

    # The stored decision is unchanged...
    assert tcl_bridge.eval_ok("isbansticky a:earlyacct") == "1"

    # ...while a ban stored now is evaluated with ISUPPORT in hand.
    tcl_bridge.eval_ok("newban a:lateacct testuser comment")
    assert tcl_bridge.eval_ok("isbansticky a:lateacct") == "0"
