"""User flag parsing, implication and conflict rules (src/flags.c).

Driven through the Tcl `chattr` command, which round-trips a flag string
through `break_down_flags` → `user_sanity_check` / `sanity_check` →
`build_flags` (src/tcluser.c:83). Reading a user's flags back with
`chattr <handle>` therefore exercises the whole parse/normalise/serialise
path in one call.

Assertions compare *sets of flag characters*, not literal strings. The
order `build_flags` emits and whether it prefixes a `+` are formatting
details that would make these tests brittle without testing anything
useful; what matters is which flags ended up set.

The implication rules asserted below come from `sanity_check`
(src/flags.c:244-277). They are not arbitrary — for example the existing
`test_tcl_matchattr.py` fixture sets `+jlmoptx`, which is exactly the
closure of `+m` under those rules.

These are characterization tests. If one fails, read `sanity_check`
before changing the assertion: the failure may be a real regression.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient

HANDLE = "flagtest"


def global_flags(bridge: BridgeClient, handle: str = HANDLE) -> set[str]:
    """Return the user's global flags as a set of characters.

    `chattr <handle>` with no change argument returns the current flag
    string. Any leading '+' and any channel section after '|' are stripped
    so callers can compare sets without depending on output formatting.
    """
    raw = bridge.eval_ok(f"chattr {handle}")
    global_part = raw.split("|")[0]
    return set(global_part.lstrip("+-"))


@pytest.fixture
def user(tcl_bridge: BridgeClient) -> str:
    """A freshly added user with no flags."""
    tcl_bridge.eval_ok(f"adduser {HANDLE}")
    return HANDLE


# ---------- basic set and clear ----------


def test_single_flag_is_stored(tcl_bridge: BridgeClient, user: str) -> None:
    """Adding a flag with no implications stores exactly that flag."""
    tcl_bridge.eval_ok(f"chattr {user} +f")
    assert "f" in global_flags(tcl_bridge)


def test_flag_can_be_removed(tcl_bridge: BridgeClient, user: str) -> None:
    """Removing a previously set flag clears it."""
    tcl_bridge.eval_ok(f"chattr {user} +f")
    assert "f" in global_flags(tcl_bridge)
    tcl_bridge.eval_ok(f"chattr {user} -f")
    assert "f" not in global_flags(tcl_bridge)


def test_unknown_flag_character_is_ignored(
    tcl_bridge: BridgeClient, user: str
) -> None:
    """A character outside the recognised sets is discarded, not rejected.

    `break_down_flags` only handles a-z, A-Z and 0-9; anything else falls
    through the switch with no effect and `chattr` reports no error. This
    matches the silent-accept behaviour already pinned in
    test_tcl_matchattr.py.
    """
    before = global_flags(tcl_bridge)
    tcl_bridge.eval_ok(f"chattr {user} +!")
    assert global_flags(tcl_bridge) == before


def test_uppercase_flag_is_a_user_defined_flag(
    tcl_bridge: BridgeClient, user: str
) -> None:
    """An uppercase letter sets a user-defined flag, stored separately
    from the built-in lowercase ones."""
    tcl_bridge.eval_ok(f"chattr {user} +B")
    assert "B" in global_flags(tcl_bridge)


@pytest.mark.parametrize(
    ("digit", "letter"),
    [("0", "A"), ("1", "B"), ("9", "J")],
)
def test_digit_flags_are_aliases_for_user_defined_flags(
    tcl_bridge: BridgeClient, user: str, digit: str, letter: str
) -> None:
    """A digit is an alternative spelling for one of the first ten
    user-defined flags, and reads back as the equivalent letter.

    `break_down_flags` maps 0-9 onto bits 0-9 of udef_global — its own
    comment says "Map 0->9 to A->K for glob/chan so they are not lost"
    (src/flags.c:1057). `flag2str` then serialises those bits starting at
    'A', so `+9` sets bit 9 and comes back as 'J'. Note the comment's
    "A->K" spans eleven letters for ten digits and is off by one; the
    behaviour is A-J.
    """
    tcl_bridge.eval_ok(f"chattr {user} +{digit}")
    flags = global_flags(tcl_bridge)
    assert letter in flags, f"+{digit} gave {flags}, expected {letter}"
    assert digit not in flags


def test_digit_and_equivalent_letter_are_the_same_flag(
    tcl_bridge: BridgeClient, user: str
) -> None:
    """Setting a flag by digit and clearing it by its letter works, because
    both spellings address the same bit."""
    tcl_bridge.eval_ok(f"chattr {user} +1")
    assert "B" in global_flags(tcl_bridge)
    tcl_bridge.eval_ok(f"chattr {user} -B")
    assert "B" not in global_flags(tcl_bridge)


# ---------- implication rules (sanity_check) ----------


def test_owner_implies_master(tcl_bridge: BridgeClient, user: str) -> None:
    """Owner cannot exist without master ("Can't be owner without also
    being master", src/flags.c:262)."""
    tcl_bridge.eval_ok(f"chattr {user} +n")
    flags = global_flags(tcl_bridge)
    assert "n" in flags
    assert "m" in flags


def test_master_implies_botmaster_op_and_janitor(
    tcl_bridge: BridgeClient, user: str
) -> None:
    """Master implies botnet master, op and janitor (src/flags.c:265)."""
    tcl_bridge.eval_ok(f"chattr {user} +m")
    flags = global_flags(tcl_bridge)
    assert {"m", "t", "o", "j"} <= flags


def test_botmaster_implies_partyline(tcl_bridge: BridgeClient, user: str) -> None:
    """Botnet master requires partyline access (src/flags.c:268)."""
    tcl_bridge.eval_ok(f"chattr {user} +t")
    assert {"t", "p"} <= global_flags(tcl_bridge)


def test_janitor_implies_file_area(tcl_bridge: BridgeClient, user: str) -> None:
    """Janitors can use the file area (src/flags.c:271)."""
    tcl_bridge.eval_ok(f"chattr {user} +j")
    assert {"j", "x"} <= global_flags(tcl_bridge)


def test_op_implies_halfop(tcl_bridge: BridgeClient, user: str) -> None:
    """Ops are also halfops (src/flags.c:274)."""
    tcl_bridge.eval_ok(f"chattr {user} +o")
    assert {"o", "l"} <= global_flags(tcl_bridge)


def test_master_closure_matches_documented_set(
    tcl_bridge: BridgeClient, user: str
) -> None:
    """Applying `+m` alone yields the full implied set `jlmoptx`.

    Chaining the rules: m → t, o, j; t → p; j → x; o → l. This is the
    same set the matchattr suite sets up by hand, so a change here would
    silently alter that suite's premise too.
    """
    tcl_bridge.eval_ok(f"chattr {user} +m")
    assert global_flags(tcl_bridge) == set("jlmoptx")


# ---------- conflict rules (sanity_check) ----------


@pytest.mark.parametrize(
    ("conflicting", "cancelled"),
    [
        ("od", ("o", "d")),   # op vs deop
        ("lr", ("l", "r")),   # halfop vs dehalfop
        ("vq", ("v", "q")),   # voice vs quiet
        ("gq", ("g", "q")),   # gvoice vs quiet
    ],
)
def test_conflicting_flags_cancel_each_other(
    tcl_bridge: BridgeClient,
    user: str,
    conflicting: str,
    cancelled: tuple[str, ...],
) -> None:
    """Mutually exclusive flags set together cancel BOTH, rather than one
    winning (src/flags.c:249-259).

    Note `+od` leaves neither op nor deop set. That is deliberate in
    sanity_check and is worth pinning, because "last one wins" is the
    behaviour most people would guess.
    """
    tcl_bridge.eval_ok(f"chattr {user} +{conflicting}")
    flags = global_flags(tcl_bridge)
    for flag in cancelled:
        assert flag not in flags, f"+{conflicting} left {flag} set: {flags}"


def test_autoop_and_deop_cancel(tcl_bridge: BridgeClient, user: str) -> None:
    """Auto-op and deop set together cancel both (src/flags.c:253)."""
    tcl_bridge.eval_ok(f"chattr {user} +ad")
    flags = global_flags(tcl_bridge)
    assert "a" not in flags
    assert "d" not in flags


def test_bot_flag_strips_human_only_flags(
    tcl_bridge: BridgeClient, user: str
) -> None:
    """A bot record cannot hold partyline, master, common or owner.

    src/flags.c:246-248 clears all four when USER_BOT is present, since
    those grant human privileges a linked bot must not have. Set via
    `addbot`, because `chattr` refuses to add or remove `b` on the fly
    (src/tcluser.c:135-137).
    """
    tcl_bridge.eval_ok("addbot flagbot 1.2.3.4")
    tcl_bridge.eval_ok("chattr flagbot +mp")
    flags = global_flags(tcl_bridge, "flagbot")
    assert "b" in flags
    assert not ({"p", "m", "c", "n"} & flags), f"bot kept human flags: {flags}"


# ---------- combined and ordering behaviour ----------


def test_plus_and_minus_in_one_call(tcl_bridge: BridgeClient, user: str) -> None:
    """A single change string may both add and remove flags."""
    tcl_bridge.eval_ok(f"chattr {user} +fk")
    assert {"f", "k"} <= global_flags(tcl_bridge)
    tcl_bridge.eval_ok(f"chattr {user} +w-f")
    flags = global_flags(tcl_bridge)
    assert "w" in flags
    assert "f" not in flags
    assert "k" in flags


def test_removal_wins_over_addition_in_same_call(
    tcl_bridge: BridgeClient, user: str
) -> None:
    """When a flag appears on both sides of one change, removal wins.

    `user_sanity_check` computes `(*atr | pls_atr) & ~min_atr`
    (src/flags.c:296), so the minus set is applied last.
    """
    tcl_bridge.eval_ok(f"chattr {user} +f-f")
    assert "f" not in global_flags(tcl_bridge)


def test_flags_survive_reread(tcl_bridge: BridgeClient, user: str) -> None:
    """Reading flags twice returns the same set — `chattr` with no change
    argument is a pure read and must not mutate the record."""
    tcl_bridge.eval_ok(f"chattr {user} +fk")
    first = global_flags(tcl_bridge)
    second = global_flags(tcl_bridge)
    assert first == second
