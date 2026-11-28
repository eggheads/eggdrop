"""RFC 2811: channel modes and the three mask lists.

RFC 2811 section 4 defines the channel modes and section 4.3 the ban,
exception and invitation lists. This file drives each of them through the
mock server and checks Eggdrop tracks the result.

The mask-list numerics matter as much as the MODE messages: a client
learns the current lists by asking, and the replies come back as

    367 / 368   ban list / end of ban list          (section 4.3)
    348 / 349   exception list / end of list        (section 4.3.1)
    346 / 347   invitation list / end of list       (section 4.3.2)

Eggdrop binds all six (irc.mod/chan.c), so each is a real code path.

Section 4.2 modes tested below: `i` invite-only, `m` moderated, `n` no
external messages, `p` private, `s` secret, `t` op-only topic, `k` key,
`l` limit, plus the status modes from section 4.1.

Conformance tests: a failure means Eggdrop mis-tracks a documented mode.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd
from support.waiters import wait_for

CHANMODES = "beI,k,l,imnpstaqr"


def mode_has(tcl_bridge: BridgeClient, chan: str, ch: str, timeout: float = 5.0):
    wait_for(
        lambda: ch in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=timeout,
        description=f"mode {ch} present on {chan}",
    )


def mode_lacks(tcl_bridge: BridgeClient, chan: str, ch: str, timeout: float = 5.0):
    wait_for(
        lambda: ch not in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=timeout,
        description=f"mode {ch} absent from {chan}",
    )


# ---------- section 4.2: modes taking no argument ----------


@pytest.mark.parametrize(
    ("flag", "meaning"),
    [
        ("i", "invite only"),
        ("m", "moderated"),
        ("n", "no messages from outside"),
        ("p", "private"),
        ("s", "secret"),
        ("t", "topic settable by operators only"),
    ],
)
def test_argumentless_channel_mode_tracked(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    flag: str,
    meaning: str,
) -> None:
    """Each argumentless channel mode from RFC 2811 section 4.2 is tracked
    when set and cleared when removed."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":o!o@m MODE {chan} +{flag}")
    mode_has(tcl_bridge, chan, flag)

    mock_ircd.send(f":o!o@m MODE {chan} -{flag}")
    mode_lacks(tcl_bridge, chan, flag)


def test_several_argumentless_modes_at_once(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Several argumentless modes set in one message are all recorded."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m MODE {chan} +imnst")
    for flag in "imnst":
        mode_has(tcl_bridge, chan, flag)


def test_private_and_secret_are_independent(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """`p` and `s` are separate modes and are tracked separately.

    RFC 2811 sections 4.2.6 and 4.2.7 define them distinctly, even though
    many servers treat setting one as clearing the other.
    """
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m MODE {chan} +p")
    mode_has(tcl_bridge, chan, "p")
    mock_ircd.send(f":o!o@m MODE {chan} -p+s")
    mode_has(tcl_bridge, chan, "s")
    mode_lacks(tcl_bridge, chan, "p")


# ---------- section 4.2.1 / 4.2.10: modes taking an argument ----------


def test_key_is_kept_when_set_and_dropped_when_removed(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """The channel key is stored with the mode and gone once unset.

    Section 4.2.11: `k` takes an argument in both directions, unlike `l`.
    """
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":o!o@m MODE {chan} +k thekey")
    wait_for(
        lambda: "thekey" in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=5.0,
        description="key recorded",
    )
    mock_ircd.send(f":o!o@m MODE {chan} -k thekey")
    wait_for(
        lambda: "thekey" not in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=5.0,
        description="key cleared",
    )


def test_limit_takes_an_argument_only_when_set(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """The user limit takes an argument when set and none when cleared.

    Section 4.2.10. A parser that expects an argument for `-l` will
    consume the next parameter and misalign everything after it.
    """
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")

    mock_ircd.send(f":o!o@m MODE {chan} +l 25")
    wait_for(
        lambda: "25" in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=5.0,
        description="limit recorded",
    )
    # -l takes no argument; the +o after it must still find "alice".
    mock_ircd.send(f":o!o@m MODE {chan} -l+o alice")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isop alice "{chan}"') == "1",
        timeout=5.0,
        description="op granted after argumentless -l",
    )
    mode_lacks(tcl_bridge, chan, "l")


def test_key_and_limit_together_consume_arguments_in_order(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """`+kl` consumes its two arguments left to right."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m MODE {chan} +kl mykey 99")
    wait_for(
        lambda: "mykey" in tcl_bridge.eval_ok(f'getchanmode "{chan}"')
        and "99" in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=5.0,
        description="key and limit both recorded",
    )


# ---------- section 4.3: ban list ----------


def test_ban_set_by_mode_appears_in_ban_list(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A ban set by MODE is tracked in the channel ban list."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m MODE {chan} +b *!*@banned.example")
    wait_for(
        lambda: "banned.example" in tcl_bridge.eval_ok(f'chanbans "{chan}"'),
        timeout=5.0,
        description="ban tracked",
    )


def test_ban_removed_by_mode_leaves_the_list(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Unsetting a ban removes it from the tracked list."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m MODE {chan} +b *!*@gone.example")
    wait_for(
        lambda: "gone.example" in tcl_bridge.eval_ok(f'chanbans "{chan}"'),
        timeout=5.0,
        description="ban tracked",
    )
    mock_ircd.send(f":o!o@m MODE {chan} -b *!*@gone.example")
    wait_for(
        lambda: "gone.example" not in tcl_bridge.eval_ok(f'chanbans "{chan}"'),
        timeout=5.0,
        description="ban removed",
    )


def test_several_bans_in_one_mode_message(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Three bans in a single MODE are all recorded, each with its own mask."""
    drive_registration(
        mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}", "MODES=3"]
    )
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(
        f":o!o@m MODE {chan} +bbb *!*@one.example *!*@two.example *!*@three.example"
    )
    for host in ("one.example", "two.example", "three.example"):
        wait_for(
            lambda h=host: h in tcl_bridge.eval_ok(f'chanbans "{chan}"'),
            timeout=5.0,
            description=f"ban on {host} tracked",
        )


def test_ban_list_numerics_populate_the_list(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Bans learned from 367 replies are tracked, ending at 368.

    This is how a client discovers bans that predate its arrival, so a
    client that only tracks MODE messages has an incomplete list.
    """
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(
        f":mock.test 367 TestBot {chan} *!*@preexisting.example oper 1700000000"
    )
    mock_ircd.send(f":mock.test 368 TestBot {chan} :End of Channel Ban List")
    wait_for(
        lambda: "preexisting.example" in tcl_bridge.eval_ok(f'chanbans "{chan}"'),
        timeout=5.0,
        description="ban learned from 367",
    )


# ---------- section 4.3.1: exception list ----------


def test_exception_set_by_mode_is_tracked(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A ban exception set by MODE lands on the exception list, not the
    ban list (section 4.3.1)."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m MODE {chan} +e *!*@except.example")
    wait_for(
        lambda: "except.example" in tcl_bridge.eval_ok(f'chanexempts "{chan}"'),
        timeout=5.0,
        description="exception tracked",
    )
    assert "except.example" not in tcl_bridge.eval_ok(f'chanbans "{chan}"')


def test_exception_list_numerics_populate_the_list(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Exceptions learned from 348 replies are tracked, ending at 349."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(
        f":mock.test 348 TestBot {chan} *!*@olde.example oper 1700000000"
    )
    mock_ircd.send(f":mock.test 349 TestBot {chan} :End of Channel Exception List")
    wait_for(
        lambda: "olde.example" in tcl_bridge.eval_ok(f'chanexempts "{chan}"'),
        timeout=5.0,
        description="exception learned from 348",
    )


# ---------- section 4.3.2: invitation list ----------


def test_invitation_mask_set_by_mode_is_tracked(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An invitation mask lands on the invite list (section 4.3.2)."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":o!o@m MODE {chan} +I *!*@invited.example")
    wait_for(
        lambda: "invited.example" in tcl_bridge.eval_ok(f'chaninvites "{chan}"'),
        timeout=5.0,
        description="invitation mask tracked",
    )
    assert "invited.example" not in tcl_bridge.eval_ok(f'chanbans "{chan}"')


def test_invitation_list_numerics_populate_the_list(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Invitation masks learned from 346 replies are tracked, ending at 347."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(
        f":mock.test 346 TestBot {chan} *!*@oldi.example oper 1700000000"
    )
    mock_ircd.send(f":mock.test 347 TestBot {chan} :End of Channel Invite List")
    wait_for(
        lambda: "oldi.example" in tcl_bridge.eval_ok(f'chaninvites "{chan}"'),
        timeout=5.0,
        description="invitation mask learned from 346",
    )


def test_the_three_lists_do_not_bleed_into_each_other(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Setting all three mask types leaves each on its own list only."""
    drive_registration(
        mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}", "MODES=3"]
    )
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(
        f":o!o@m MODE {chan} +beI *!*@b.example *!*@e.example *!*@i.example"
    )
    wait_for(
        lambda: "b.example" in tcl_bridge.eval_ok(f'chanbans "{chan}"'),
        timeout=5.0,
        description="all three masks applied",
    )
    bans = tcl_bridge.eval_ok(f'chanbans "{chan}"')
    exempts = tcl_bridge.eval_ok(f'chanexempts "{chan}"')
    invites = tcl_bridge.eval_ok(f'chaninvites "{chan}"')

    assert "e.example" not in bans and "i.example" not in bans
    assert "b.example" not in exempts and "i.example" not in exempts
    assert "b.example" not in invites and "e.example" not in invites


# ---------- section 4.2: mode query and 324 ----------


def test_channel_mode_numeric_sets_current_modes(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Numeric 324 reports the channel's current modes and is applied.

    This is how a client learns the modes in force at join time, before
    any MODE message arrives.
    """
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":mock.test 324 TestBot {chan} +mnt")
    for flag in "mnt":
        mode_has(tcl_bridge, chan, flag)


def test_channel_mode_numeric_with_arguments(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A 324 carrying key and limit arguments applies both."""
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":mock.test 324 TestBot {chan} +ntkl akey 30")
    wait_for(
        lambda: "akey" in tcl_bridge.eval_ok(f'getchanmode "{chan}"')
        and "30" in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=5.0,
        description="324 arguments applied",
    )


# ---------- section 4.1: channel operator status ----------


def test_status_mode_on_a_user_not_present_is_harmless(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Granting op to a nick the bot does not know does not corrupt state.

    Servers can legitimately send this during a netsplit heal, so the
    following mode change must still apply to the right user.
    """
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    mock_ircd.send(f":o!o@m MODE {chan} +o ghostuser")
    mock_ircd.send(f":o!o@m MODE {chan} +o alice")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isop alice "{chan}"') == "1",
        timeout=5.0,
        description="op applied to the right user",
    )


def test_bot_tracks_its_own_operator_status(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """The bot notices when it is opped and deopped itself.

    Section 4.1: operator status governs what the bot may do, so its own
    status has to be tracked accurately.
    """
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={CHANMODES}"])
    chan = drive_join_with_names(mock_ircd, "TestBot")
    assert tcl_bridge.eval_ok(f'botisop "{chan}"') == "0"

    mock_ircd.send(f":o!o@m MODE {chan} +o TestBot")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'botisop "{chan}"') == "1",
        timeout=5.0,
        description="bot sees its own op",
    )
    mock_ircd.send(f":o!o@m MODE {chan} -o TestBot")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'botisop "{chan}"') == "0",
        timeout=5.0,
        description="bot sees its own deop",
    )
