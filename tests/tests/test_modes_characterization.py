"""Characterization tests for core mode state and formats (arbmodes3 step 0).

These pin *current* behaviour before the ISUPPORT-driven mode refactor:
they must stay green through every refactor step (ARCHITECTURE.md test plan
section A — A1, A2, A3, A4, A5, A27 — plus the B0 MODES clamp test).

Where the refactor deliberately allows a change (e.g. getchanmode flag
*ordering*, D-CHM5), the assertion is written to the stable contract (set
membership + arg order), not the incidental detail.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import (
    drive_join_with_names,
    drive_registration,
    get_mode_log,
    install_mode_log,
    wait_for_isupport,
)
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


def wait_onchan(bridge: BridgeClient, nick: str, chan: str) -> None:
    wait_for(
        lambda: bridge.eval_ok(f'onchan {nick} "{chan}"') == "1",
        timeout=5.0,
        description=f"{nick} to appear on {chan}",
    )


# ---------- A1: getchanmode format ----------


def test_a1_getchanmode_flags_key_limit(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """324 `+ntkl secret 42` → getchanmode shows the n/t/k/l flags (set
    membership — ordering may change per D-CHM5) followed by key then limit
    in that exact argument order."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot")

    mock_ircd.send(f":mock.test 324 TestBot {chan} +ntkl secret 42")
    wait_for(
        lambda: "secret" in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=5.0,
        description="324 to apply key",
    )

    mode_str = tcl_bridge.eval_ok(f'getchanmode "{chan}"')
    parts = mode_str.split()
    assert len(parts) == 3, mode_str
    flags, key, limit = parts
    assert flags.startswith("+"), mode_str
    assert set("ntkl") <= set(flags), mode_str
    assert key == "secret", mode_str
    assert limit == "42", mode_str


def test_a1_botisop_from_names_prefix(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """The bot's own @ in NAMES (via the WHO reply) sets botisop."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot plain")
    wait_onchan(tcl_bridge, "plain", chan)
    assert tcl_bridge.eval_ok(f'botisop "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'botishalfop "{chan}"') == "0"


@pytest.mark.parametrize(
    ("bot_token", "expected_op", "expected_halfop", "expected_voice"),
    [
        ("@TestBot", "1", "0", "0"),
        ("%TestBot", "0", "1", "0"),
        ("+TestBot", "0", "0", "1"),
        ("TestBot", "0", "0", "0"),
    ],
)
def test_a1_bot_status_helpers_match_member_status(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
    bot_token: str,
    expected_op: str,
    expected_halfop: str,
    expected_voice: str,
) -> None:
    """botisop/botishalfop/botisvoice are compatibility wrappers for
    the bot's literal o/h/v member status, not rank-based capability.
    """
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, f"{bot_token} plain")
    wait_onchan(tcl_bridge, "plain", chan)

    for bot_cmd, member_cmd, expected in (
        ("botisop", "isop", expected_op),
        ("botishalfop", "ishalfop", expected_halfop),
        ("botisvoice", "isvoice", expected_voice),
    ):
        bot_value = tcl_bridge.eval_ok(f'{bot_cmd} "{chan}"')
        member_value = tcl_bridge.eval_ok(f'{member_cmd} TestBot "{chan}"')
        assert bot_value == expected, bot_cmd
        assert member_value == expected, member_cmd
        assert bot_value == member_value, (bot_cmd, member_cmd)


# ---------- A2: bind mode args + wasop visibility ----------


def test_a2_bind_mode_args_and_wasop(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """One MODE burst `+o-o+v+b-l alice alice bob *!*@x` fires five binds in
    order with the historic arg conventions:
      - prefix modes pass the nick,
      - +b passes the mask,
      - -l passes "" (quirk),
      - inside the +o bind `wasop` is still 0; inside the -o bind it is 1.
    """
    drive_registration(mock_ircd)
    install_mode_log(tcl_bridge)
    chan = drive_join_with_names(mock_ircd, "@TestBot alice bob")
    wait_onchan(tcl_bridge, "bob", chan)

    mock_ircd.send(
        f":someop!su@sh.example.com MODE {chan} +o-o+v+b-l alice alice bob *!*@x"
    )
    wait_for(
        lambda: len(get_mode_log(tcl_bridge)) >= 5,
        timeout=5.0,
        description="five mode binds to fire",
    )

    log = get_mode_log(tcl_bridge)
    assert log == [
        "+o|alice|0",
        "-o|alice|1",
        "+v|bob|0",
        "+b|*!*@x|0",
        "-l||0",
    ], log


# ---------- A3: member status/metadata from WHO (plain 352 and WHOX 354) ----


def test_a3_status_from_plain_352(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Plain WHO (352) flags map to op/halfop/voice/wasop; `G` marks away,
    the 005 BOT=B char marks isircbot."""
    drive_registration(mock_ircd, isupport_tokens=["BOT=B"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @opper %hopper +voicy plain")
    wait_onchan(tcl_bridge, "plain", chan)

    assert tcl_bridge.eval_ok(f'isop opper "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'wasop opper "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'ishalfop hopper "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isvoice voicy "{chan}"') == "1"
    for cmd in ("isop", "ishalfop", "isvoice", "wasop"):
        assert tcl_bridge.eval_ok(f'{cmd} plain "{chan}"') == "0", cmd

    # Stray 352s update metadata: G = away, B (from BOT=B) = ircbot.
    mock_ircd.send(
        f":mock.test 352 TestBot {chan} au ah.example.com mock.test awayguy G :0 x"
    )
    mock_ircd.send(
        f":mock.test 352 TestBot {chan} bu bh.example.com mock.test botguy HB :0 x"
    )
    wait_onchan(tcl_bridge, "botguy", chan)
    assert tcl_bridge.eval_ok(f'isaway awayguy "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isircbot botguy "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isaway botguy "{chan}"') == "0"


def test_a3_status_from_whox_354(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """With WHOX advertised the bot uses 354 replies; same status mapping."""
    drive_registration(mock_ircd, isupport_tokens=["WHOX"])
    chan = drive_join_with_names(mock_ircd, "@TestBot @opper %hopper +voicy plain")
    wait_onchan(tcl_bridge, "plain", chan)

    assert tcl_bridge.eval_ok(f'isop opper "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'ishalfop hopper "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isvoice voicy "{chan}"') == "1"
    for cmd in ("isop", "ishalfop", "isvoice"):
        assert tcl_bridge.eval_ok(f'{cmd} plain "{chan}"') == "0", cmd


# A4 (userhost-in-names) lives in test_modes_userhost_in_names.py — the cap
# must be advertised in CAP LS at MockIrcd construction time, which needs a
# module-local mock_ircd fixture override.


# ---------- A5: netsplit WAS-state + stopnethack ----------


def test_a5_netsplit_wasop_and_stopnethack(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """stopnethack-mode 2 (wasop test): after a netsplit rejoin, a server
    re-op of a previously-opped member is accepted; a server op of a member
    who was never op is reverted (FAKEOP path)."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "@TestBot @alice bob", chanmodes_324="+nt")
    wait_onchan(tcl_bridge, "bob", chan)
    tcl_bridge.eval_ok(f"channel set {chan} stopnethack-mode 2")

    # alice (op) netsplits and rejoins; her op is gone but WASOP survives.
    mock_ircd.send(":alice!u@h.example.com QUIT :irc1.test irc2.test")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchansplit alice "{chan}"') == "1",
        timeout=5.0,
        description="alice to be marked netsplit",
    )
    mock_ircd.send(f":alice!u@h.example.com JOIN :{chan}")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchansplit alice "{chan}"') == "0",
        timeout=5.0,
        description="alice to rejoin from netsplit",
    )

    # Server re-ops alice (wasop) then bob (never op). Only bob's op is
    # reverted; the first outbound MODE must be the -o for bob.
    mock_ircd.send(f":mock.test MODE {chan} +o alice")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'isop alice "{chan}"') == "1",
        timeout=5.0,
        description="alice to be re-opped by the server",
    )
    mock_ircd.send(f":mock.test MODE {chan} +o bob")
    lines = mock_ircd.drain_until(
        lambda line: line.startswith("MODE "), timeout=5.0
    )
    assert lines[-1] == f"MODE {chan} -o bob", lines


# ---------- A27: Undernet key=* triggers a MODE re-ask once opped ----------


def test_a27_key_star_reasks_modes_when_opped(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """324 `+k *` (Undernet hides the key from non-ops) sets the asked-modes
    flag; when the bot later gets opped it re-asks `MODE #chan`."""
    drive_registration(mock_ircd)
    chan = drive_join_with_names(mock_ircd, "TestBot @someop")
    wait_onchan(tcl_bridge, "someop", chan)

    mock_ircd.send(f":mock.test 324 TestBot {chan} +k *")
    wait_for(
        lambda: "k" in tcl_bridge.eval_ok(f'getchanmode "{chan}"').split()[0],
        timeout=5.0,
        description="324 +k * to be processed",
    )

    mock_ircd.send(f":someop!su@sh.example.com MODE {chan} +o TestBot")
    lines = mock_ircd.drain_until(
        lambda line: line == f"MODE {chan}", timeout=5.0
    )
    assert lines[-1] == f"MODE {chan}"


# ---------- B0: MODES up to 32 is honored by the queue ----------


def test_b0_modes_isupport_honored_above_legacy_six(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """A server advertising MODES=20 is honored by the 32-slot queue:
    short masks can exceed the legacy six-mode line cap and all masks
    still arrive with aligned arguments."""
    drive_registration(mock_ircd, isupport_tokens=["MODES=20"])
    wait_for_isupport(tcl_bridge, "MODES", "20")
    chan = drive_join_with_names(mock_ircd, "@TestBot", chanmodes_324="+nt")

    for i in range(12):
        tcl_bridge.eval_ok(f'pushmode "{chan}" +b "m{i}!*@*"')
    tcl_bridge.eval_ok(f'flushmode "{chan}"')

    eggdrop_proc.assert_alive()
    line = mock_ircd.drain_until(
        lambda line: line.startswith(f"MODE {chan} "), timeout=5.0
    )[-1]
    words = line.split()
    modes = words[2].lstrip("+-")
    args = words[3:]
    assert len(modes) > 6, line
    assert len(args) == len(modes), line
    assert args == [f"m{i}!*@*" for i in range(12)]
