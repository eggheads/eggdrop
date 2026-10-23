"""Tests for arbitrary chan-modes (isupport PREFIX + CHANMODES) handling.

Each test is self-contained — the realistic isupport strings used (drawn
from the top-3 most-popular advertised values at
https://stats.ircdocs.horse/isupport/) appear inline so the test reads as
a complete story.

Coverage:
- 005 parsing of PREFIX and CHANMODES → debug log + isupport state
- `.status all` on the partyline reflects what was parsed
- Raw 324 (RPL_CHANNELMODEIS) with mixed known/unknown modes after join
- Inbound MODE messages mixing prefix modes, hardcoded modes, and unknown
  modes — Eggdrop must skip what it doesn't know but still apply known
  ones in the right slots (op grant, key after junk modes).
"""

from __future__ import annotations

import re

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

# ---------- 005 parsing: debug log + isupport state ----------


def test_005_parses_prefix_qaohv_and_chanmodes_top1(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Most-popular PREFIX (qaohv, 123 networks) + most-popular CHANMODES.

    Each prefix mode and each LIST/KEY chanmode produces a corresponding
    "Learned mode type: ..." debug line, and isupport state matches.
    """
    prefix = "(qaohv)~&@%+"
    chanmodes = "beI,kLf,l,psmntirzMQNRTOVKDdGPZSCc"

    drive_registration(
        mock_ircd, isupport_tokens=[f"PREFIX={prefix}", f"CHANMODES={chanmodes}"]
    )
    wait_for_isupport(tcl_bridge, "PREFIX", prefix)
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)

    log = eggdrop_proc.log_path.read_text()
    # Each prefix letter → symbol pair must appear in the debug log.
    for letter, symbol in [("q", "~"), ("a", "&"), ("o", "@"), ("h", "%"), ("v", "+")]:
        pat = (
            rf"Learned mode type: \+{letter} type Prefix, "
            rf"prefixchar {re.escape(symbol)}"
        )
        assert re.search(pat, log), f"missing prefix debug line for +{letter} {symbol}"
    # CHANMODES sections: LIST=beI, KEY=kLf.
    for letter in "beI":
        assert re.search(rf"Learned mode type: \+{letter} type List(?!.)", log), (
            f"missing List debug line for +{letter}"
        )
    for letter in "kLf":
        assert re.search(rf"Learned mode type: \+{letter} type Key(?!.)", log), (
            f"missing Key debug line for +{letter}"
        )


def test_005_parses_prefix_ohv_and_chanmodes_top2(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Second-most-popular PREFIX (47 networks) + second-most-popular CHANMODES."""
    prefix = "(ohv)@%+"
    chanmodes = "beI,kfL,lj,psmntirRcOAQKVCuzNSMTG"

    drive_registration(
        mock_ircd, isupport_tokens=[f"PREFIX={prefix}", f"CHANMODES={chanmodes}"]
    )
    wait_for_isupport(tcl_bridge, "PREFIX", prefix)
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)

    log = eggdrop_proc.log_path.read_text()
    for letter, symbol in [("o", "@"), ("h", "%"), ("v", "+")]:
        pat = (
            rf"Learned mode type: \+{letter} type Prefix, "
            rf"prefixchar {re.escape(symbol)}"
        )
        assert re.search(pat, log), f"missing prefix debug line for +{letter} {symbol}"
    # CHANMODES sections: LIST=beI, KEY=kfL.
    for letter in "beI":
        assert re.search(rf"Learned mode type: \+{letter} type List(?!.)", log)
    for letter in "kfL":
        assert re.search(rf"Learned mode type: \+{letter} type Key(?!.)", log)


def test_005_parses_prefix_ov_and_chanmodes_top3(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """Basic PREFIX (op + voice only) + CHANMODES with q/a as LIST modes
    instead of prefix modes."""
    prefix = "(ov)@+"
    chanmodes = "beIqa,kLf,l,psmntirzMQNRTOVKDdGPZS"

    drive_registration(
        mock_ircd, isupport_tokens=[f"PREFIX={prefix}", f"CHANMODES={chanmodes}"]
    )
    wait_for_isupport(tcl_bridge, "PREFIX", prefix)
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)

    log = eggdrop_proc.log_path.read_text()
    for letter, symbol in [("o", "@"), ("v", "+")]:
        pat = (
            rf"Learned mode type: \+{letter} type Prefix, "
            rf"prefixchar {re.escape(symbol)}"
        )
        assert re.search(pat, log), f"missing prefix debug line for +{letter} {symbol}"
    # CHANMODES sections: LIST=beIqa (note 'q' and 'a' are LIST here, not prefix), KEY=kLf.
    for letter in "beIqa":
        assert re.search(rf"Learned mode type: \+{letter} type List(?!.)", log)
    for letter in "kLf":
        assert re.search(rf"Learned mode type: \+{letter} type Key(?!.)", log)


# ---------- use-exempts / use-invites derive from CHANMODES ----------


def test_use_exempts_and_invites_set_when_e_and_I_in_list(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`use-exempts` / `use-invites` are turned on when 'e' and 'I' are in the
    LIST section of CHANMODES (the typical case for major IRCds)."""
    chanmodes = "beI,kLf,l,psmntirzMQNRTOVKDdGPZSCc"  # 'e' and 'I' both in LIST
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={chanmodes}"])
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)

    assert tcl_bridge.eval_ok("set ::use-exempts") == "1"
    assert tcl_bridge.eval_ok("set ::use-invites") == "1"


def test_use_exempts_off_when_e_not_in_list(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """If 'e' is missing from the LIST section, use-exempts goes to 0
    (use-invites stays on because 'I' is still in LIST)."""
    chanmodes = "bI,k,l,imnpst"  # no 'e' in list section
    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={chanmodes}"])
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)

    assert tcl_bridge.eval_ok("set ::use-exempts") == "0"
    assert tcl_bridge.eval_ok("set ::use-invites") == "1"


# ---------- .status all on the partyline ----------


@pytest.mark.partyline
def test_status_all_reports_parsed_isupport(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """`.status all` partyline output contains an `isupport:` line including
    the PREFIX and CHANMODES we sent in 005."""
    prefix = "(qaohv)~&@%+"
    chanmodes = "beI,kLf,l,psmntirzMQNRTOVKDdGPZSCc"

    drive_registration(
        mock_ircd, isupport_tokens=[f"PREFIX={prefix}", f"CHANMODES={chanmodes}"]
    )
    wait_for_isupport(tcl_bridge, "PREFIX", prefix)

    # Mark a snapshot so we only scan output produced after sending the cmd.
    snapshot_offset = len(eggdrop_proc.stdout_text())
    eggdrop_proc.send_partyline(".status all")

    def has_isupport_line() -> bool:
        new_output = eggdrop_proc.stdout_text()[snapshot_offset:]
        return any(
            "isupport:" in line and "PREFIX=" in line and "CHANMODES=" in line
            for line in new_output.splitlines()
        )

    wait_for(
        has_isupport_line,
        timeout=5.0,
        description="`.status all` to print the isupport: line",
    )

    new_output = eggdrop_proc.stdout_text()[snapshot_offset:]
    assert f"PREFIX={prefix}" in new_output
    assert f"CHANMODES={chanmodes}" in new_output


# ---------- raw 324 with mixed modes ----------


def test_got324_skips_unknown_mode_but_applies_key(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """324 `+fk test_f test_k` — `f` (forward, common on Charybdis) is unknown
    to Eggdrop but isupport says it has a parameter; `f` is skipped, the key
    `test_k` still applies."""
    prefix = "(ohv)@%+"
    # 'f' lives in the KEY section here (kfL), so isupport says it takes an arg.
    chanmodes = "beI,kfL,lj,psmntirRcOAQKVCuzNSMTG"

    drive_registration(
        mock_ircd, isupport_tokens=[f"PREFIX={prefix}", f"CHANMODES={chanmodes}"]
    )
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)

    mock_ircd.send(f":mock.test 324 TestBot {chan} +fk test_f test_k")

    wait_for(
        lambda: "test_k" in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=5.0,
        description="324 to apply key=test_k",
    )

    chanmode_str = tcl_bridge.eval_ok(f'getchanmode "{chan}"')
    # Format is "+<flags>k <key>" — verify 'k' is set, key is correct,
    # and 'f' (unknown) does not appear in the flags.
    flags, _, key = chanmode_str.partition(" ")
    assert "k" in flags, chanmode_str
    assert "f" not in flags, chanmode_str
    assert key.strip() == "test_k", chanmode_str


def test_got324_conflict_eggdrop_says_noargs_isupport_says_args(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """If isupport puts a mode Eggdrop hardcodes as no-arg into a section
    with args, got324 logs a warning and skips that mode (consuming its
    arg). The next mode in line still gets parsed correctly.

    Constructed CHANMODES: 'q' (Eggdrop hardcodes it as no-arg / quiet) is
    placed in the LIST section (with-arg). Sending `324 +qk arg_q test_k`
    must skip the `q arg_q` pair entirely and still apply `+k test_k`.
    """
    chanmodes = "beIq,k,l,imnpst"  # 'q' in LIST → isupport says it takes arg

    drive_registration(mock_ircd, isupport_tokens=[f"CHANMODES={chanmodes}"])
    chan = drive_join_with_names(mock_ircd, "@TestBot")
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)

    mock_ircd.send(f":mock.test 324 TestBot {chan} +qk arg_q test_k")

    wait_for(
        lambda: "test_k" in tcl_bridge.eval_ok(f'getchanmode "{chan}"'),
        timeout=5.0,
        description="324 to apply key=test_k after skipped +q",
    )

    log = eggdrop_proc.log_path.read_text()
    assert re.search(
        r"Eggdrop assumes mode change \+q has no parameter but isupport says yes",
        log,
    ), "expected the +q conflict warning in the log"


# ---------- inbound MODE while joined ----------


def test_gotmode_op_via_prefix_after_unknown_mode_with_arg(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """MODE `+ofk alice test_f test_k` on a joined channel:
    - alice gets opped (prefix mode +o consumes its arg correctly),
    - 'f' is consumed-and-discarded (unknown to Eggdrop but isupport says
      it takes an arg),
    - 'k' still applies with `test_k`.
    """
    prefix = "(ohv)@%+"
    chanmodes = "beI,kfL,lj,psmntirRcOAQKVCuzNSMTG"  # 'f' is KEY-type here

    drive_registration(
        mock_ircd, isupport_tokens=[f"PREFIX={prefix}", f"CHANMODES={chanmodes}"]
    )
    chan = drive_join_with_names(mock_ircd, "@TestBot alice")
    wait_for_isupport(tcl_bridge, "CHANMODES", chanmodes)
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan alice "{chan}"') == "1",
        timeout=5.0,
        description="alice to appear in chanlist",
    )

    mock_ircd.send(f":someop!u@h MODE {chan} +ofk alice test_f test_k")

    wait_for(
        lambda: tcl_bridge.eval_ok(f'isop alice "{chan}"') == "1",
        timeout=5.0,
        description="alice to be opped via prefix +o",
    )
    chanmode_str = tcl_bridge.eval_ok(f'getchanmode "{chan}"')
    flags, _, key = chanmode_str.partition(" ")
    assert key.strip() == "test_k", chanmode_str
    assert "f" not in flags, chanmode_str


def test_gotmode_extended_prefix_modes_qa_consume_args_correctly(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """With PREFIX=(qaohv)~&@%+, MODE `+qav nick1 nick2 nick3` consumes args
    for each prefix mode (owner/admin/voice).

    Eggdrop only tracks op/halfop/voice flags by default, but the parser
    must still consume all three args correctly so the next mode in line
    isn't shifted. Verified by checking voice on the third nick.
    """
    prefix = "(qaohv)~&@%+"
    chanmodes = "beI,kLf,l,psmntirzMQNRTOVKDdGPZSCc"

    drive_registration(
        mock_ircd, isupport_tokens=[f"PREFIX={prefix}", f"CHANMODES={chanmodes}"]
    )
    chan = drive_join_with_names(mock_ircd, "@TestBot owner_user admin_user voice_user")
    wait_for_isupport(tcl_bridge, "PREFIX", prefix)
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan voice_user "{chan}"') == "1",
        timeout=5.0,
        description="voice_user to appear in chanlist",
    )

    mock_ircd.send(f":someop!u@h MODE {chan} +qav owner_user admin_user voice_user")

    wait_for(
        lambda: tcl_bridge.eval_ok(f'isvoice voice_user "{chan}"') == "1",
        timeout=5.0,
        description="voice_user to be voiced after +qav consumed all 3 args",
    )


def test_names_with_extended_prefix_grants_op_when_opchars_includes_it(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    """NAMES line with `~` (owner) prefix grants op when `opchars` contains it.

    Eggdrop's op-recognition uses the `opchars` set; for networks with
    extended prefixes admins typically configure `opchars "~&@"`. With that
    config, a `~user` in NAMES (and the corresponding WHO 352 with `H~` in
    flags) lands as op.
    """
    prefix = "(qaohv)~&@%+"

    drive_registration(mock_ircd, isupport_tokens=[f"PREFIX={prefix}"])
    # Set opchars to include owner/admin symbols. Must happen before the JOIN
    # is driven because that's when NAMES + WHO 352 entries are processed.
    tcl_bridge.eval_ok('set opchars "~&@"')

    chan = drive_join_with_names(mock_ircd, "@TestBot ~bigboss +regular")
    wait_for(
        lambda: tcl_bridge.eval_ok(f'onchan bigboss "{chan}"') == "1",
        timeout=5.0,
        description="bigboss to appear in chanlist",
    )
    assert tcl_bridge.eval_ok(f'isop bigboss "{chan}"') == "1"
    assert tcl_bridge.eval_ok(f'isvoice regular "{chan}"') == "1"
