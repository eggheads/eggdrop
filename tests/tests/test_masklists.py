"""Ban, exempt and invite list management (src/mod/channels.mod/userchan.c).

Exercises the storage layer behind the mask lists: adding, removing,
sticking, expiry classification and list retrieval, for all three list
types and for both the global and per-channel scopes.

The existing test_tcl_iscmds.py covers only the `is*` lookups. This file
covers the rest of the surface — `new*`, `kill*`, `stick`/`unstick`,
`isperm*`, `match*` and `*list` — which is where most of userchan.c's
uncovered lines are.

Naming note: `newban` adds to the global list, `newchanban` adds to a
single channel's list, and the two are independent. Several tests below
exist purely to pin that separation, because the command names are close
enough to be easy to confuse.

Characterization tests: they record current behaviour so refactoring is
safe. Read the C before changing an assertion.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient

CHAN = "#test"


def q(bridge: BridgeClient, cmd: str) -> str:
    """Evaluate a Tcl command, brace-quoting nothing — caller does that."""
    return bridge.eval_ok(cmd)


# ---------- global list lifecycle ----------


@pytest.mark.parametrize(
    ("kind", "add", "kill", "check"),
    [
        ("ban", "newban", "killban", "isban"),
        ("exempt", "newexempt", "killexempt", "isexempt"),
        ("invite", "newinvite", "killinvite", "isinvite"),
    ],
)
def test_global_mask_add_then_remove(
    tcl_bridge: BridgeClient, kind: str, add: str, kill: str, check: str
) -> None:
    """A global mask can be added and then removed, and the lookup reflects
    each state."""
    mask = f"*!*@{kind}.example"
    assert tcl_bridge.eval_ok(f"{check} {{{mask}}}") == "0"
    tcl_bridge.eval_ok(f"{add} {{{mask}}} tester comment")
    assert tcl_bridge.eval_ok(f"{check} {{{mask}}}") == "1"
    tcl_bridge.eval_ok(f"{kill} {{{mask}}}")
    assert tcl_bridge.eval_ok(f"{check} {{{mask}}}") == "0"


@pytest.mark.parametrize("kill", ["killban", "killexempt", "killinvite"])
def test_killing_absent_mask_reports_failure(
    tcl_bridge: BridgeClient, kill: str
) -> None:
    """Removing a mask that was never added reports failure rather than
    raising an error."""
    assert tcl_bridge.eval_ok(f"{kill} {{*!*@never.added}}") == "0"


def test_adding_same_mask_twice_does_not_duplicate(
    tcl_bridge: BridgeClient,
) -> None:
    """Adding an identical mask a second time replaces the existing entry
    rather than creating a duplicate."""
    mask = "*!*@dup.example"
    tcl_bridge.eval_ok(f"newban {{{mask}}} tester first")
    tcl_bridge.eval_ok(f"newban {{{mask}}} tester second")
    entries = [e for e in tcl_bridge.eval_ok("banlist").split("} {") if mask in e]
    assert len(entries) == 1, f"expected one entry, got {entries}"


# ---------- global vs channel separation ----------


def test_global_and_channel_bans_are_separate_lists(
    tcl_bridge: BridgeClient,
) -> None:
    """A ban added globally does not appear in a channel's own list, and
    vice versa.

    `banlist` with no argument reads global_bans; with a channel argument
    it reads chan->bans (tclchan.c:1784). They are distinct structures.
    """
    tcl_bridge.eval_ok("newban {*!*@globalonly.example} tester g")
    tcl_bridge.eval_ok(f"newchanban {CHAN} {{*!*@chanonly.example}} tester c")

    global_list = tcl_bridge.eval_ok("banlist")
    chan_list = tcl_bridge.eval_ok(f"banlist {CHAN}")

    assert "globalonly.example" in global_list
    assert "globalonly.example" not in chan_list
    assert "chanonly.example" in chan_list
    assert "chanonly.example" not in global_list


def test_killchanban_leaves_global_ban_intact(tcl_bridge: BridgeClient) -> None:
    """Removing a channel ban does not touch a global ban with the same
    mask."""
    mask = "*!*@both.example"
    tcl_bridge.eval_ok(f"newban {{{mask}}} tester g")
    tcl_bridge.eval_ok(f"newchanban {CHAN} {{{mask}}} tester c")
    tcl_bridge.eval_ok(f"killchanban {CHAN} {{{mask}}}")

    assert tcl_bridge.eval_ok(f"isban {{{mask}}}") == "1"
    assert "both.example" not in tcl_bridge.eval_ok(f"banlist {CHAN}")


def test_banlist_rejects_unknown_channel(tcl_bridge: BridgeClient) -> None:
    """Asking for the ban list of a channel the bot doesn't know is an
    error, not an empty list."""
    ok, result = tcl_bridge.eval("banlist #nosuchchannel")
    assert not ok
    assert "invalid channel" in result


# ---------- sticky handling ----------


def test_stick_and_unstick_global_ban(tcl_bridge: BridgeClient) -> None:
    """A ban can be made sticky after the fact and unstuck again."""
    mask = "*!*@sticky.example"
    tcl_bridge.eval_ok(f"newban {{{mask}}} tester c")
    assert tcl_bridge.eval_ok(f"isbansticky {{{mask}}}") == "0"

    assert tcl_bridge.eval_ok(f"stick {{{mask}}}") == "1"
    assert tcl_bridge.eval_ok(f"isbansticky {{{mask}}}") == "1"

    assert tcl_bridge.eval_ok(f"unstick {{{mask}}}") == "1"
    assert tcl_bridge.eval_ok(f"isbansticky {{{mask}}}") == "0"


def test_stickban_is_an_alias_for_stick(tcl_bridge: BridgeClient) -> None:
    """`stickban` and `unstickban` are alternative names for the same
    commands (tclchan.c:2481-2484 all point at tcl_stick)."""
    mask = "*!*@alias.example"
    tcl_bridge.eval_ok(f"newban {{{mask}}} tester c")
    tcl_bridge.eval_ok(f"stickban {{{mask}}}")
    assert tcl_bridge.eval_ok(f"isbansticky {{{mask}}}") == "1"
    tcl_bridge.eval_ok(f"unstickban {{{mask}}}")
    assert tcl_bridge.eval_ok(f"isbansticky {{{mask}}}") == "0"


def test_sticking_absent_ban_reports_failure(tcl_bridge: BridgeClient) -> None:
    """Sticking a mask that isn't on any list reports failure."""
    assert tcl_bridge.eval_ok("stick {*!*@notthere.example}") == "0"


def test_ban_created_sticky_by_option(tcl_bridge: BridgeClient) -> None:
    """The `sticky` option on creation sets the flag immediately."""
    mask = "*!*@born-sticky.example"
    tcl_bridge.eval_ok(f"newban {{{mask}}} tester c 0 sticky")
    assert tcl_bridge.eval_ok(f"isbansticky {{{mask}}}") == "1"


# ---------- permanence ----------


def test_zero_lifetime_makes_ban_permanent(tcl_bridge: BridgeClient) -> None:
    """A lifetime of 0 marks the ban permanent."""
    mask = "*!*@perm.example"
    tcl_bridge.eval_ok(f"newban {{{mask}}} tester c 0")
    assert tcl_bridge.eval_ok(f"ispermban {{{mask}}}") == "1"


def test_nonzero_lifetime_is_not_permanent(tcl_bridge: BridgeClient) -> None:
    """A ban given a finite lifetime is not reported as permanent."""
    mask = "*!*@temp.example"
    tcl_bridge.eval_ok(f"newban {{{mask}}} tester c 3600")
    assert tcl_bridge.eval_ok(f"ispermban {{{mask}}}") == "0"


# ---------- matching an address against stored masks ----------


@pytest.mark.parametrize(
    ("stored", "address", "expected"),
    [
        ("*!*@host.example", "nick!user@host.example", "1"),
        ("*!*@*.example", "nick!user@host.example", "1"),
        ("nick!*@*", "nick!user@host.example", "1"),
        ("*!*@other.example", "nick!user@host.example", "0"),
        ("other!*@*", "nick!user@host.example", "0"),
    ],
)
def test_matchban_against_stored_mask(
    tcl_bridge: BridgeClient, stored: str, address: str, expected: str
) -> None:
    """`matchban` reports whether an address is covered by any stored ban,
    as opposed to `isban`, which compares the mask literally."""
    tcl_bridge.eval_ok(f"newban {{{stored}}} tester c")
    assert tcl_bridge.eval_ok(f"matchban {{{address}}}") == expected


def test_matchban_differs_from_isban(tcl_bridge: BridgeClient) -> None:
    """`isban` needs the exact mask; `matchban` matches an address against
    it. Confusing the two is a common scripting error, so pin both."""
    tcl_bridge.eval_ok("newban {*!*@covered.example} tester c")
    addr = "nick!user@covered.example"
    assert tcl_bridge.eval_ok(f"matchban {{{addr}}}") == "1"
    assert tcl_bridge.eval_ok(f"isban {{{addr}}}") == "0"


@pytest.mark.parametrize(
    ("add", "match"),
    [("newexempt", "matchexempt"), ("newinvite", "matchinvite")],
)
def test_match_works_for_exempts_and_invites(
    tcl_bridge: BridgeClient, add: str, match: str
) -> None:
    """Address matching works the same way for exempt and invite lists."""
    tcl_bridge.eval_ok(f"{add} {{*!*@match.example}} tester c")
    assert tcl_bridge.eval_ok(f"{match} {{nick!user@match.example}}") == "1"
    assert tcl_bridge.eval_ok(f"{match} {{nick!user@nope.example}}") == "0"


# ---------- list retrieval ----------


def test_banlist_entry_contains_mask_creator_and_comment(
    tcl_bridge: BridgeClient,
) -> None:
    """Each list entry carries the mask along with the comment and creator
    supplied at creation time."""
    tcl_bridge.eval_ok("newban {*!*@fields.example} bobcreator somecomment")
    entry = tcl_bridge.eval_ok("banlist")
    assert "fields.example" in entry
    assert "somecomment" in entry
    assert "bobcreator" in entry


def test_empty_banlist_is_empty(tcl_bridge: BridgeClient) -> None:
    """With nothing stored, the list is empty rather than an error."""
    assert tcl_bridge.eval_ok("banlist").strip() == ""


@pytest.mark.parametrize(
    ("add", "listcmd"),
    [
        ("newexempt", "exemptlist"),
        ("newinvite", "invitelist"),
    ],
)
def test_exempt_and_invite_lists_retrieve_entries(
    tcl_bridge: BridgeClient, add: str, listcmd: str
) -> None:
    """Exempt and invite lists retrieve their own entries."""
    tcl_bridge.eval_ok(f"{add} {{*!*@listed.example}} tester c")
    assert "listed.example" in tcl_bridge.eval_ok(listcmd)


def test_lists_do_not_leak_between_types(tcl_bridge: BridgeClient) -> None:
    """Bans, exempts and invites are three separate stores; adding to one
    does not affect the others."""
    tcl_bridge.eval_ok("newban {*!*@onlyban.example} tester c")
    assert "onlyban.example" not in tcl_bridge.eval_ok("exemptlist")
    assert "onlyban.example" not in tcl_bridge.eval_ok("invitelist")
    assert tcl_bridge.eval_ok("isexempt {*!*@onlyban.example}") == "0"
    assert tcl_bridge.eval_ok("isinvite {*!*@onlyban.example}") == "0"
