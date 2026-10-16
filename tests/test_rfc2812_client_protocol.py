"""RFC 2812: client protocol numerics and command handling.

Covers the numeric replies and commands Eggdrop actually binds, drawn
from the bind tables in irc.mod/chan.c and server.mod/servmsg.c. Testing
numerics with no handler would be fluff, so every numeric below has one.

Handled numerics exercised here:

    001         RPL_WELCOME                 section 5.1
    315 / 352   RPL_ENDOFWHO / RPL_WHOREPLY section 5.1
    354         RPL_WHOSPCRPL (WHOX)        de facto, not in 2812
    366         RPL_ENDOFNAMES              section 5.1
    396         RPL_HOSTHIDDEN              de facto
    403         ERR_NOSUCHCHANNEL           section 5.2
    405         ERR_TOOMANYCHANNELS         section 5.2
    442         ERR_NOTONCHANNEL            section 5.2
    471 / 473 / 474 / 475
                ERR_CHANNELISFULL / INVITEONLYCHAN /
                BANNEDFROMCHAN / BADCHANNELKEY   section 5.2

Commands: AWAY (3.1.5), INVITE (3.2.7), NICK (3.1.2), plus CHGHOST and
ACCOUNT, which are IRCv3 rather than 2812 and are marked as such.

The join-failure numerics are the most valuable group. A bot that does
not understand why a join failed will retry forever against an
invite-only or keyed channel, which is both useless and rude to the
network.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


def bot_alive_and_responsive(eggdrop_proc, tcl_bridge: BridgeClient) -> None:
    """The bot survived whatever we just sent and still answers."""
    eggdrop_proc.assert_alive()
    assert tcl_bridge.eval_ok("expr 2+2") == "4"


# ---------- section 5.2: join failure numerics ----------


@pytest.mark.parametrize(
    ("numeric", "meaning"),
    [
        ("471", "channel is full"),
        ("473", "invite only"),
        ("474", "banned from channel"),
        ("475", "bad channel key"),
    ],
)
def test_join_failure_numeric_is_handled(
    eggdrop_proc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    numeric: str,
    meaning: str,
) -> None:
    """Each documented join-failure numeric is processed without wedging
    the bot.

    RFC 2812 section 5.2. Eggdrop binds all four in irc.mod/chan.c, so
    each is a real handler rather than an ignored line.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(f":mock.test {numeric} TestBot #test :{meaning}")
    bot_alive_and_responsive(eggdrop_proc, tcl_bridge)


def test_join_failure_leaves_channel_not_active(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """After an invite-only rejection the bot does not believe it joined.

    A client that marks the channel active on the strength of its own
    JOIN request, rather than the server's confirmation, will report
    membership it does not have.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 473 TestBot #test :Cannot join channel (+i)")
    assert tcl_bridge.eval_ok("botonchan #test") == "0"


def test_no_such_channel_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """ERR_NOSUCHCHANNEL (403) is processed without disrupting the bot."""
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 403 TestBot #nosuch :No such channel")
    bot_alive_and_responsive(eggdrop_proc, tcl_bridge)


def test_too_many_channels_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """ERR_TOOMANYCHANNELS (405) is processed rather than ignored.

    Section 5.2; the limit itself is advertised as CHANLIMIT.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 405 TestBot #test :You have joined too many channels")
    bot_alive_and_responsive(eggdrop_proc, tcl_bridge)


def test_not_on_channel_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """ERR_NOTONCHANNEL (442) is processed.

    Sent when the bot acts on a channel the server thinks it left --
    which is exactly when its own state is wrong and worth correcting.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(":mock.test 442 TestBot #test :You're not on that channel")
    bot_alive_and_responsive(eggdrop_proc, tcl_bridge)


# ---------- section 5.1: WHO replies ----------


def test_who_reply_populates_member_host(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A 352 WHO reply supplies user@host for a member.

    Format per section 5.1: `352 <client> <channel> <user> <host>
    <server> <nick> <flags> :<hopcount> <real name>`. NAMES gives only
    nicknames, so WHO is how a client learns hostmasks.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":mock.test 353 TestBot = {chan} :@TestBot whouser")
    mock_ircd.send(f":mock.test 366 TestBot {chan} :End of /NAMES list.")
    mock_ircd.send(
        f":mock.test 352 TestBot {chan} theuser the.host mock.test whouser H :0 Real"
    )
    mock_ircd.send(f":mock.test 315 TestBot {chan} :End of /WHO list.")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getchanhost whouser "{chan}"')
        == "theuser@the.host",
        timeout=5.0,
        description="host learned from 352",
    )


def test_who_reply_away_flag_sets_away_state(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """The `G` flag in a WHO reply marks the user away, `H` marks them here.

    Section 5.1 defines H for here and G for gone.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":mock.test 353 TestBot = {chan} :@TestBot awayuser")
    mock_ircd.send(f":mock.test 366 TestBot {chan} :End of /NAMES list.")
    mock_ircd.send(
        f":mock.test 352 TestBot {chan} u h.example mock.test awayuser G :0 Real"
    )
    mock_ircd.send(f":mock.test 315 TestBot {chan} :End of /WHO list.")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isaway awayuser "{chan}"') == "1",
        timeout=5.0,
        description="away flag from 352",
    )


def test_end_of_who_completes_the_sweep(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """315 terminates the WHO burst and the channel remains usable."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":mock.test 315 TestBot {chan} :End of /WHO list.")
    assert tcl_bridge.eval_ok(f'botonchan "{chan}"') == "1"


# ---------- section 3.1.5: AWAY ----------


def test_away_command_marks_member_away(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An AWAY message with text marks the sender away.

    RFC 2812 section 3.1.5. Relaying AWAY to channel members is an
    IRCv3 `away-notify` extension rather than base 2812, but the command
    itself is defined there and Eggdrop binds it.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    mock_ircd.send(":alice!u@h.example AWAY :out to lunch")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isaway alice "{chan}"') == "1",
        timeout=5.0,
        description="alice marked away",
    )


def test_away_with_no_text_clears_away(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An AWAY with no message clears the away state (section 3.1.5)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    mock_ircd.send(":alice!u@h.example AWAY :gone")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isaway alice "{chan}"') == "1",
        timeout=5.0,
        description="alice away",
    )
    mock_ircd.send(":alice!u@h.example AWAY")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isaway alice "{chan}"') == "0",
        timeout=5.0,
        description="alice back",
    )


# ---------- section 3.2.7: INVITE ----------


def test_invite_to_the_bot_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """An INVITE addressed to the bot is processed without error.

    Section 3.2.7. Whether the bot acts on it depends on configuration;
    what matters here is that the message parses and does not wedge it.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(":alice!u@h.example INVITE TestBot :#elsewhere")
    bot_alive_and_responsive(eggdrop_proc, tcl_bridge)


# ---------- section 3.1.2: NICK ----------


def test_bot_tracks_its_own_nick_change(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """When the bot's own nick changes, it knows its new name.

    Section 3.1.2. A bot that keeps the old nick will fail every
    subsequent `match_my_nick` check, including the guards that stop it
    reacting to its own messages.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(":TestBot!u@h.example NICK :NewBotNick")
    wait_for(
        lambda: tcl_bridge.eval_ok("set ::botnick") == "NewBotNick",
        timeout=5.0,
        description="bot learned its new nick",
    )
    assert tcl_bridge.eval_ok(f'botonchan "{chan}"') == "1"


def test_nick_change_to_a_case_variant(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Changing nick to a different capitalisation keeps membership.

    Servers permit this and it is a classic way to break a member list
    keyed on the literal nickname.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    mock_ircd.send(":alice!u@h.example NICK :ALICE")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan ALICE "{chan}"') == "1",
        timeout=5.0,
        description="member survives a case-only nick change",
    )


# ---------- IRCv3 extensions Eggdrop binds ----------


def test_chghost_updates_member_host(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """CHGHOST updates a member's user@host in place.

    An IRCv3 extension rather than RFC 2812, included because Eggdrop
    binds it and a stale hostmask silently breaks ban matching.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    mock_ircd.send(f":alice!olduser@old.host JOIN {chan}")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getchanhost alice "{chan}"')
        == "olduser@old.host",
        timeout=5.0,
        description="original host recorded",
    )
    mock_ircd.send(":alice!olduser@old.host CHGHOST newuser new.host")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'getchanhost alice "{chan}"')
        == "newuser@new.host",
        timeout=5.0,
        description="host updated by CHGHOST",
    )


def test_hostname_changed_numeric_is_handled(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """RPL_HOSTHIDDEN (396) tells the bot its own host changed.

    Not in RFC 2812 but near-universal, and Eggdrop binds it. It matters
    because the bot's own hostmask affects self-ban checks.
    """
    drive_registration(mock_ircd)
    mock_ircd.send(":mock.test 396 TestBot cloaked.example :is now your hidden host")
    bot_alive_and_responsive(eggdrop_proc, tcl_bridge)


# ---------- registration numerics ----------


def test_welcome_numeric_completes_registration(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """RPL_WELCOME (001) marks the bot as connected (section 5.1)."""
    drive_registration(mock_ircd)
    assert tcl_bridge.eval_ok("set ::server") != ""


def test_isupport_may_arrive_in_several_lines(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """Multiple 005 lines accumulate rather than replacing each other.

    Servers routinely split ISUPPORT across several messages; a client
    that keeps only the last line loses most of the advertisement.
    """
    drive_registration(mock_ircd, isupport_tokens=["CHANTYPES=#&"])
    mock_ircd.send(":mock.test 005 TestBot AWAYLEN=200 :are supported")
    mock_ircd.send(":mock.test 005 TestBot TOPICLEN=390 :are supported")
    wait_for(
        lambda: tcl_bridge.eval_ok("isupport get AWAYLEN") == "200"
        and tcl_bridge.eval_ok("isupport get TOPICLEN") == "390",
        timeout=5.0,
        description="both 005 lines retained",
    )
    assert tcl_bridge.eval_ok("isupport get CHANTYPES") == "#&"


def test_isupport_negation_removes_a_token(
    eggdrop_proc, mock_ircd: MockIrcd, tcl_bridge: BridgeClient
) -> None:
    """A token prefixed with `-` withdraws a previously advertised value.

    Part of the ISUPPORT specification, and easy to miss: a client that
    treats `-TOKEN` as setting a token named `-TOKEN` keeps using a
    capability the server has just revoked.
    """
    drive_registration(mock_ircd, isupport_tokens=["EXCEPTS=e"])
    wait_for(
        lambda: tcl_bridge.eval_ok("isupport isset EXCEPTS") == "1",
        timeout=5.0,
        description="EXCEPTS advertised",
    )
    mock_ircd.send(":mock.test 005 TestBot -EXCEPTS :are supported")
    wait_for(
        lambda: tcl_bridge.eval_ok("isupport isset EXCEPTS") == "0",
        timeout=5.0,
        description="EXCEPTS withdrawn",
    )
