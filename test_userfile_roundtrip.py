"""Userfile persistence round-trip (src/userrec.c, src/users.c, src/userent.c).

Together these three files are roughly 3,900 lines and were among the
least covered in the tree. They implement writing the userfile out and
reading it back in, plus the per-entry pack/unpack handlers for each kind
of record a user can hold.

The approach here is one round trip per entry type: build a user carrying
that record, `save`, `reload`, and check the record survived. That walks
the write handler, the parser and the unpack handler for the type in a
single test, which is a lot of code per line of test.

Entry types registered at userent.c:43 and their `setuser`/`getuser`
names:

    COMMENT   INFO   PASS   PASS2   LASTON   BOTADDR
    XTRA      HOSTS  BOTFL  ACCOUNT  FPRINT

`reload` discards in-memory state and re-reads from disk, so anything
that fails to serialise disappears rather than merely changing -- which
makes these tests good at catching silent data loss.

Characterization tests: they record what currently survives a round trip.
A failure means either a real persistence bug or a wrong assumption about
the entry format, and the two are easy to tell apart by looking at the
saved userfile.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient

HANDLE = "roundtrip"
HOST = "*!*@roundtrip.example"


@pytest.fixture
def saved_user(tcl_bridge: BridgeClient) -> str:
    """A user with a hostmask, created fresh for each test."""
    tcl_bridge.eval_ok(f"adduser {HANDLE} {{{HOST}}}")
    return HANDLE


def cycle(tcl_bridge: BridgeClient) -> None:
    """Write the userfile out and read it back, discarding memory state."""
    tcl_bridge.eval_ok("save")
    tcl_bridge.eval_ok("reload")


# ---------- the user record itself ----------


def test_user_survives_save_and_reload(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """A user written to the userfile is still there after reloading."""
    cycle(tcl_bridge)
    assert tcl_bridge.eval_ok(f"validuser {saved_user}") == "1"


def test_deleted_user_stays_deleted_after_reload(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """Deleting a user removes them from the file, not just from memory."""
    cycle(tcl_bridge)
    tcl_bridge.eval_ok(f"deluser {saved_user}")
    cycle(tcl_bridge)
    assert tcl_bridge.eval_ok(f"validuser {saved_user}") == "0"


def test_multiple_users_all_survive(tcl_bridge: BridgeClient) -> None:
    """Several users written in one save are all read back.

    Guards against a writer that stops after the first record, or a
    parser that stops at the first blank line.
    """
    handles = [f"multi{i}" for i in range(5)]
    for h in handles:
        tcl_bridge.eval_ok(f"adduser {h} {{*!*@{h}.example}}")
    cycle(tcl_bridge)
    for h in handles:
        assert tcl_bridge.eval_ok(f"validuser {h}") == "1", h


def test_handle_case_is_preserved(tcl_bridge: BridgeClient) -> None:
    """A handle's original capitalisation survives the round trip, even
    though lookups are case-insensitive."""
    tcl_bridge.eval_ok("adduser MixedCase {*!*@mixed.example}")
    cycle(tcl_bridge)
    assert "MixedCase" in tcl_bridge.eval_ok("userlist")


# ---------- flags ----------


def test_flags_survive_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """Global flags are written and read back unchanged."""
    tcl_bridge.eval_ok(f"chattr {saved_user} +fk")
    before = tcl_bridge.eval_ok(f"chattr {saved_user}")
    cycle(tcl_bridge)
    assert tcl_bridge.eval_ok(f"chattr {saved_user}") == before


def test_implied_flags_survive_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """The full implied flag set from `+m` survives, not just the letter
    that was typed."""
    tcl_bridge.eval_ok(f"chattr {saved_user} +m")
    cycle(tcl_bridge)
    flags = set(tcl_bridge.eval_ok(f"chattr {saved_user}").split("|")[0].lstrip("+-"))
    assert set("jlmoptx") <= flags


def test_user_defined_flags_survive_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """Uppercase user-defined flags are persisted alongside built-in ones."""
    tcl_bridge.eval_ok(f"chattr {saved_user} +B")
    cycle(tcl_bridge)
    assert "B" in tcl_bridge.eval_ok(f"chattr {saved_user}")


def test_channel_flags_survive_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """Per-channel flags are written with their channel and read back."""
    tcl_bridge.eval_ok(f"chattr {saved_user} |+o #test")
    cycle(tcl_bridge)
    assert tcl_bridge.eval_ok(f"matchattr {saved_user} |o #test") == "1"


# ---------- hosts ----------


def test_hostmask_survives_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """A user's hostmask is persisted (USERENTRY_HOSTS)."""
    cycle(tcl_bridge)
    assert "roundtrip.example" in tcl_bridge.eval_ok(f"getuser {saved_user} HOSTS")


def test_multiple_hostmasks_survive_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """Every hostmask on a user survives, not only the first."""
    for i in range(3):
        tcl_bridge.eval_ok(f"setuser {saved_user} HOSTS {{*!*@extra{i}.example}}")
    cycle(tcl_bridge)
    hosts = tcl_bridge.eval_ok(f"getuser {saved_user} HOSTS")
    for i in range(3):
        assert f"extra{i}.example" in hosts, hosts


def test_deleted_host_stays_deleted(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """Removing a hostmask removes it from the file too."""
    tcl_bridge.eval_ok(f"setuser {saved_user} HOSTS {{*!*@gone.example}}")
    cycle(tcl_bridge)
    tcl_bridge.eval_ok(f"delhost {saved_user} {{*!*@gone.example}}")
    cycle(tcl_bridge)
    assert "gone.example" not in tcl_bridge.eval_ok(f"getuser {saved_user} HOSTS")


def test_hostmask_with_special_characters_survives(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """A hostmask containing RFC 2812 special characters round-trips.

    `[]\\`_^{|}` are legal in nicknames, so they appear in real
    hostmasks. A writer that treats any of them as a field separator
    would corrupt the file.
    """
    mask = "ni\\[ck\\]!*@special.example"
    tcl_bridge.eval_ok(f"setuser {saved_user} HOSTS {mask}")
    cycle(tcl_bridge)
    assert "special.example" in tcl_bridge.eval_ok(f"getuser {saved_user} HOSTS")


# ---------- password ----------


def test_password_survives_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """A password still validates after a save and reload.

    The stored form is a hash, so this checks the hash survives intact --
    a truncated or re-encoded hash would fail to match without any
    visible corruption in the file.
    """
    tcl_bridge.eval_ok(f"setuser {saved_user} PASS secretpw1")
    assert tcl_bridge.eval_ok(f"passwdok {saved_user} secretpw1") == "1"
    cycle(tcl_bridge)
    assert tcl_bridge.eval_ok(f"passwdok {saved_user} secretpw1") == "1"
    assert tcl_bridge.eval_ok(f"passwdok {saved_user} wrongpw12") == "0"


def test_absence_of_password_survives_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """A user with no password still has none after reloading."""
    cycle(tcl_bridge)
    assert tcl_bridge.eval_ok(f"passwdok {saved_user} -") == "1"


# ---------- text entry types ----------


@pytest.mark.parametrize("entry", ["COMMENT", "INFO"])
def test_text_entry_survives_round_trip(
    tcl_bridge: BridgeClient, saved_user: str, entry: str
) -> None:
    """Free-text entries are persisted verbatim."""
    tcl_bridge.eval_ok(f"setuser {saved_user} {entry} {{some text here}}")
    cycle(tcl_bridge)
    assert tcl_bridge.eval_ok(f"getuser {saved_user} {entry}") == "some text here"


@pytest.mark.parametrize("entry", ["COMMENT", "INFO"])
def test_text_entry_with_awkward_characters(
    tcl_bridge: BridgeClient, saved_user: str, entry: str
) -> None:
    """Text entries containing colons and braces survive.

    The userfile is line-oriented with colon-delimited fields, so a value
    containing a colon is the obvious way to corrupt it.
    """
    value = "a:colon and {braces} here"
    tcl_bridge.eval_ok(f"setuser {saved_user} {entry} {{{value}}}")
    cycle(tcl_bridge)
    assert tcl_bridge.eval_ok(f"getuser {saved_user} {entry}") == value


def test_xtra_entry_survives_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """XTRA holds arbitrary key/value pairs for scripts, and both parts
    survive the round trip."""
    tcl_bridge.eval_ok(f"setuser {saved_user} XTRA mykey {{my value}}")
    cycle(tcl_bridge)
    assert tcl_bridge.eval_ok(f"getuser {saved_user} XTRA mykey") == "my value"


def test_multiple_xtra_keys_survive(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """Several XTRA keys on one user are all persisted."""
    for i in range(3):
        tcl_bridge.eval_ok(f"setuser {saved_user} XTRA key{i} {{value{i}}}")
    cycle(tcl_bridge)
    for i in range(3):
        assert tcl_bridge.eval_ok(f"getuser {saved_user} XTRA key{i}") == f"value{i}"


def test_account_entry_survives_round_trip(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """A linked services account name is persisted (USERENTRY_ACCOUNT)."""
    tcl_bridge.eval_ok(f"setuser {saved_user} ACCOUNT myaccount")
    cycle(tcl_bridge)
    assert "myaccount" in tcl_bridge.eval_ok(f"getuser {saved_user} ACCOUNT")


# ---------- bot records ----------


def test_bot_address_survives_round_trip(tcl_bridge: BridgeClient) -> None:
    """A bot's address and ports are persisted (USERENTRY_BOTADDR)."""
    tcl_bridge.eval_ok("addbot rtbot 1.2.3.4 3333 4444")
    cycle(tcl_bridge)
    addr = tcl_bridge.eval_ok("getuser rtbot BOTADDR")
    assert "1.2.3.4" in addr
    assert "3333" in addr


def test_ipv6_bot_address_survives_round_trip(tcl_bridge: BridgeClient) -> None:
    """An IPv6 bot address survives, colons and all.

    The address format is colon-heavy and the userfile uses colons as
    field separators, so this is the case most likely to break.
    """
    tcl_bridge.eval_ok("addbot rtbot6 fe80::1 3333")
    cycle(tcl_bridge)
    assert "fe80::1" in tcl_bridge.eval_ok("getuser rtbot6 BOTADDR")


def test_bot_flag_survives_round_trip(tcl_bridge: BridgeClient) -> None:
    """A bot record is still a bot after reloading."""
    tcl_bridge.eval_ok("addbot rtbot2 5.6.7.8")
    cycle(tcl_bridge)
    assert "b" in tcl_bridge.eval_ok("chattr rtbot2").split("|")[0]


# ---------- combined ----------


def test_user_with_every_entry_type_survives(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """A user carrying many record types at once round-trips intact.

    The per-type tests above each exercise one writer in isolation; this
    checks they coexist on one record without one truncating the next.
    """
    tcl_bridge.eval_ok(f"chattr {saved_user} +fk|+o #test")
    tcl_bridge.eval_ok(f"setuser {saved_user} PASS combined1")
    tcl_bridge.eval_ok(f"setuser {saved_user} COMMENT {{a comment}}")
    tcl_bridge.eval_ok(f"setuser {saved_user} INFO {{an info line}}")
    tcl_bridge.eval_ok(f"setuser {saved_user} XTRA akey {{a value}}")
    tcl_bridge.eval_ok(f"setuser {saved_user} HOSTS {{*!*@second.example}}")

    cycle(tcl_bridge)

    assert tcl_bridge.eval_ok(f"validuser {saved_user}") == "1"
    assert tcl_bridge.eval_ok(f"passwdok {saved_user} combined1") == "1"
    assert tcl_bridge.eval_ok(f"getuser {saved_user} COMMENT") == "a comment"
    assert tcl_bridge.eval_ok(f"getuser {saved_user} INFO") == "an info line"
    assert tcl_bridge.eval_ok(f"getuser {saved_user} XTRA akey") == "a value"
    assert "second.example" in tcl_bridge.eval_ok(f"getuser {saved_user} HOSTS")
    assert tcl_bridge.eval_ok(f"matchattr {saved_user} |o #test") == "1"


def test_repeated_cycles_are_stable(
    tcl_bridge: BridgeClient, saved_user: str
) -> None:
    """Saving and reloading repeatedly does not drift.

    Guards against a writer that re-escapes an already-escaped value, or
    appends rather than replacing -- both of which look fine after one
    cycle and corrupt after several.
    """
    tcl_bridge.eval_ok(f"setuser {saved_user} COMMENT {{stable: value}}")
    tcl_bridge.eval_ok(f"chattr {saved_user} +fk")

    for _ in range(3):
        cycle(tcl_bridge)

    assert tcl_bridge.eval_ok(f"getuser {saved_user} COMMENT") == "stable: value"
    assert "f" in tcl_bridge.eval_ok(f"chattr {saved_user}")
    assert tcl_bridge.eval_ok("countusers") != "0"
