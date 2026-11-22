"""`/msg` command handlers (src/mod/irc.mod/msgcmds.c).

These are the commands any user on IRC can send the bot by private
message: `hello`, `ident`, `pass`, `op`, `voice`, `invite`, `key`,
`whois`, `die`, `jump` and the rest. The whole file had zero coverage
before this, despite being the largest untrusted-input surface the bot
exposes.

The bind table at msgcmds.c:1119 gates each command on user flags:

    die     n       owner only
    jump    m       master only
    rehash  m       reset   m       save    m
    memory  m       status  m|m
    invite  o|o     key     o|o     global or channel op
    hello   ident   pass    op      voice   who     whois   (no flags)

The authorization tests below matter more than the coverage they buy: a
regression that let an unknown user reach `die` would be about as bad as
a bug in this bot gets.

Note `learn-users` must be on for `hello` to do anything (msgcmds.c:32),
and several handlers reply with a NOTICE only when `quiet-reject` is off.
Tests that depend on either say so.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd, MockIrcdError

USER_HOST = "alice!ident@alice.example"
USER_NICK = "alice"


def send_msg(mock_ircd: MockIrcd, text: str, prefix: str = USER_HOST) -> None:
    """Send a private message to the bot as if from an IRC user."""
    mock_ircd.send(f":{prefix} PRIVMSG TestBot :{text}")


def expect_notice(
    mock_ircd: MockIrcd, contains: str, timeout: float = 10.0
) -> str:
    """Wait for a NOTICE back to the user containing `contains`."""
    return mock_ircd.drain_until(
        lambda ln: ln.startswith(f"NOTICE {USER_NICK}") and contains in ln,
        timeout=timeout,
    )[-1]


def expect_no_reply(mock_ircd: MockIrcd, timeout: float = 3.0) -> None:
    """Assert the bot sends nothing back to the user."""
    with pytest.raises(MockIrcdError):
        mock_ircd.drain_until(
            lambda ln: ln.startswith((f"NOTICE {USER_NICK}", f"PRIVMSG {USER_NICK}")),
            timeout=timeout,
        )


# ---------- authorization: privileged commands reject unknown users ----------


@pytest.mark.parametrize("command", ["die", "jump", "rehash", "save", "reset"])
def test_privileged_command_ignored_from_unknown_user(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    command: str,
) -> None:
    """A user with no record cannot invoke an owner- or master-only command.

    The bind flags in C_msg (msgcmds.c:1119) gate these on `n` or `m`. An
    unrecognised sender holds neither, so the handler must not run. This
    is the single most important property in this file: `die` shuts the
    bot down.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    send_msg(mock_ircd, command)
    expect_no_reply(mock_ircd)
    eggdrop_proc.assert_alive()


def test_die_from_unknown_user_does_not_stop_the_bot(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`die` from an unprivileged user leaves the bot running.

    Checked separately from the parametrized case above because the
    consequence is qualitatively different: the bot must still be alive
    and still answering afterwards, not merely silent.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    send_msg(mock_ircd, "die because I said so")
    expect_no_reply(mock_ircd)

    eggdrop_proc.assert_alive()
    assert tcl_bridge.eval_ok("expr 1+1") == "2"


@pytest.mark.parametrize("command", ["invite #test", "key #test"])
def test_op_only_command_ignored_from_unknown_user(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    command: str,
) -> None:
    """Commands gated on `o|o` are not available to an unknown user."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    send_msg(mock_ircd, command)
    expect_no_reply(mock_ircd)


def test_op_command_ignored_without_a_user_record(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`op` does not op a user the bot has no record for.

    `op` carries no bind flags, so the handler runs -- but it must check
    the caller's channel flags itself before issuing a mode change.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")

    send_msg(mock_ircd, f"op {chan}")

    with pytest.raises(MockIrcdError):
        mock_ircd.drain_until(
            lambda ln: ln.startswith(f"MODE {chan}") and "+o" in ln,
            timeout=3.0,
        )


# ---------- hello: user creation ----------


def test_hello_is_inert_when_learn_users_is_off(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """With `learn-users` off, `hello` creates no user and sends nothing.

    msgcmds.c:32 returns immediately unless `learn_users` or
    `make_userfile` is set. Default configurations leave both off, so an
    arbitrary IRC user cannot add themselves to the userfile.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    send_msg(mock_ircd, "hello")
    expect_no_reply(mock_ircd)
    assert tcl_bridge.eval_ok(f"validuser {USER_NICK}") == "0"


def test_hello_greets_a_known_user(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A user the bot already knows is greeted by handle rather than being
    re-created (msgcmds.c:38-43)."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    tcl_bridge.eval_ok(f"adduser {USER_NICK} {{*!*@alice.example}}")
    tcl_bridge.eval_ok("set ::learn-users 1")

    send_msg(mock_ircd, "hello")
    reply = expect_notice(mock_ircd, USER_NICK)
    assert USER_NICK in reply


# ---------- pass: setting and changing a password ----------


def test_pass_is_ignored_for_unknown_user(
    eggdrop_proc: EggdropProc, mock_ircd: MockIrcd
) -> None:
    """`pass` does nothing for a sender with no user record.

    msgcmds.c:124 returns immediately when `u` is NULL, so an unknown
    user cannot set a password on anything.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    send_msg(mock_ircd, "pass newsecret")
    expect_no_reply(mock_ircd)


def test_pass_sets_password_for_known_user(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A known user with no password can set one, and it takes effect."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")
    tcl_bridge.eval_ok(f"adduser {USER_NICK} {{*!*@alice.example}}")
    assert tcl_bridge.eval_ok(f"passwdok {USER_NICK} -") == "1"

    send_msg(mock_ircd, "pass s3cretpass")
    expect_notice(mock_ircd, "s3cretpass")

    assert tcl_bridge.eval_ok(f"passwdok {USER_NICK} s3cretpass") == "1"
    assert tcl_bridge.eval_ok(f"passwdok {USER_NICK} -") == "0"


def test_pass_requires_old_password_to_change_it(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Once a password is set, changing it requires the old one.

    msgcmds.c:139: with a password already in place, `u_pass_match` must
    succeed against the first argument before the new one is accepted.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")
    tcl_bridge.eval_ok(f"adduser {USER_NICK} {{*!*@alice.example}}")
    tcl_bridge.eval_ok(f"setuser {USER_NICK} PASS original1")

    send_msg(mock_ircd, "pass wrongone brandnew1")
    expect_notice(mock_ircd, "")

    assert tcl_bridge.eval_ok(f"passwdok {USER_NICK} original1") == "1"
    assert tcl_bridge.eval_ok(f"passwdok {USER_NICK} brandnew1") == "0"


def test_pass_changes_password_with_correct_old_one(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Supplying the current password allows it to be replaced."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")
    tcl_bridge.eval_ok(f"adduser {USER_NICK} {{*!*@alice.example}}")
    tcl_bridge.eval_ok(f"setuser {USER_NICK} PASS original1")

    send_msg(mock_ircd, "pass original1 brandnew1")
    expect_notice(mock_ircd, "brandnew1")

    assert tcl_bridge.eval_ok(f"passwdok {USER_NICK} brandnew1") == "1"
    assert tcl_bridge.eval_ok(f"passwdok {USER_NICK} original1") == "0"


def test_pass_with_no_argument_reports_whether_one_is_set(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A bare `pass` reports whether a password exists without changing it
    (msgcmds.c:127-132)."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")
    tcl_bridge.eval_ok(f"adduser {USER_NICK} {{*!*@alice.example}}")

    send_msg(mock_ircd, "pass")
    expect_notice(mock_ircd, "")
    assert tcl_bridge.eval_ok(f"passwdok {USER_NICK} -") == "1"


# ---------- ident: authenticating from a new host ----------


def test_ident_rejects_wrong_password(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A wrong password does not add the sender's host to the user record.

    This is the path by which a user on an unrecognised host proves who
    they are, so a failure to check the password here would let anyone
    attach themselves to any account.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")
    tcl_bridge.eval_ok("adduser target {*!*@known.example}")
    tcl_bridge.eval_ok("setuser target PASS realpass1")

    send_msg(mock_ircd, "ident wrongpass1 target")

    hosts = tcl_bridge.eval_ok("getuser target HOSTS")
    assert "alice.example" not in hosts


def test_ident_with_correct_password_adds_host(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """The correct password adds the sender's hostmask to the account."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")
    tcl_bridge.eval_ok("adduser target {*!*@known.example}")
    tcl_bridge.eval_ok("setuser target PASS realpass1")

    send_msg(mock_ircd, "ident realpass1 target")
    expect_notice(mock_ircd, "")

    hosts = tcl_bridge.eval_ok("getuser target HOSTS")
    assert "alice.example" in hosts


def test_ident_for_unknown_handle_does_not_create_a_user(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Identifying against a handle that doesn't exist creates nothing."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    send_msg(mock_ircd, "ident somepass nosuchhandle")
    assert tcl_bridge.eval_ok("validuser nosuchhandle") == "0"


# ---------- commands from the bot itself ----------


def test_bot_ignores_commands_from_its_own_nick(
    eggdrop_proc: EggdropProc, mock_ircd: MockIrcd
) -> None:
    """A message appearing to come from the bot's own nickname is ignored.

    Several handlers open with `match_my_nick(nick)` (msgcmds.c:35, 124,
    161). Without that check a spoofed or looped message could drive the
    bot's own privileged commands.
    """
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    send_msg(mock_ircd, "hello", prefix="TestBot!u@h.example")
    send_msg(mock_ircd, "pass newpass1", prefix="TestBot!u@h.example")
    expect_no_reply(mock_ircd)


# ---------- unknown commands ----------


def test_unknown_msg_command_is_ignored(
    eggdrop_proc: EggdropProc, mock_ircd: MockIrcd
) -> None:
    """A private message that matches no bound command produces no reply."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    send_msg(mock_ircd, "definitelynotacommand with args")
    expect_no_reply(mock_ircd)


def test_empty_privmsg_is_handled(
    eggdrop_proc: EggdropProc, mock_ircd: MockIrcd
) -> None:
    """An empty private message does not crash or provoke a reply."""
    drive_registration(mock_ircd)
    drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":{USER_HOST} PRIVMSG TestBot :")
    expect_no_reply(mock_ircd)
    eggdrop_proc.assert_alive()
