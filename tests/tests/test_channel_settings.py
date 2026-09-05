"""Channel setting storage and retrieval (src/mod/channels.mod/tclchan.c).

`tclchan.c` is the largest file in channels.mod and one of the least
covered. Most of it is the `channel set` / `channel get` setting table:
a long if/else chain parsing setting names and validating their values.

Each test here drives one setting through set-then-get. The existing
test_chanset_inputvalidation.py covers only flood-deop's validation; this
covers the storage path for the rest of the table, plus the flag settings
that share `tcl_channel_modify`.

Characterization tests. Where a value comes back in a different form than
it went in, the docstring says so rather than asserting the tidier answer.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient

CHAN = "#test"


# ---------- integer settings ----------


@pytest.mark.parametrize(
    "setting",
    [
        "ban-time",
        "exempt-time",
        "invite-time",
        "idle-kick",
        "need-limit",
    ],
)
def test_integer_setting_round_trips(
    tcl_bridge: BridgeClient, setting: str
) -> None:
    """An integer channel setting reads back the value it was given."""
    tcl_bridge.eval_ok(f'channel set {CHAN} {setting} 42')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} {setting}') == "42"


@pytest.mark.parametrize(
    "setting", ["ban-time", "exempt-time", "invite-time", "idle-kick"]
)
def test_integer_setting_accepts_zero(
    tcl_bridge: BridgeClient, setting: str
) -> None:
    """Zero is accepted and generally means the feature is switched off."""
    tcl_bridge.eval_ok(f'channel set {CHAN} {setting} 0')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} {setting}') == "0"


def test_ban_type_round_trips(tcl_bridge: BridgeClient) -> None:
    """The ban-type setting, which selects the hostmask style used when
    the bot builds a ban, stores the value given."""
    tcl_bridge.eval_ok(f'channel set {CHAN} ban-type 3')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} ban-type') == "3"


# ---------- flood settings (pairs) ----------


@pytest.mark.parametrize(
    "setting",
    ["flood-chan", "flood-ctcp", "flood-join", "flood-kick", "flood-nick"],
)
def test_flood_setting_round_trips(
    tcl_bridge: BridgeClient, setting: str
) -> None:
    """A flood limit given as count:window reads back as two space-separated
    numbers.

    Input accepts the colon form, but `channel get` renders every pair with
    simple_sprintf(s, "%d %d", ...) (tclchan.c:1168-1176), so the output
    form differs from the input form. Pinned because it surprises scripts
    that compare the value they just set.
    """
    tcl_bridge.eval_ok(f'channel set {CHAN} {setting} 5:60')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} {setting}') == "5 60"


@pytest.mark.parametrize(
    "setting",
    ["flood-chan", "flood-ctcp", "flood-join", "flood-kick", "flood-nick"],
)
def test_flood_setting_rejects_malformed_value(
    tcl_bridge: BridgeClient, setting: str
) -> None:
    """Every flood setting rejects a value that isn't two integers, not
    just flood-deop.

    test_chanset_inputvalidation.py pins this for flood-deop alone; the
    same validation should apply across the family.
    """
    ok, _ = tcl_bridge.eval(f'channel set {CHAN} {setting} notanumber')
    assert not ok


# ---------- boolean flag settings ----------


@pytest.mark.parametrize(
    "flag",
    [
        "dynamicbans",
        "dynamicexempts",
        "dynamicinvites",
        "userbans",
        "userexempts",
        "userinvites",
        "enforcebans",
        "autoop",
        "autovoice",
        "bitch",
        "greet",
        "protectops",
        "protectfriends",
        "dontkickops",
        "inactive",
        "secret",
        "shared",
        "cycle",
        "seen",
        "nodesynch",
    ],
)
def test_channel_flag_can_be_set_and_cleared(
    tcl_bridge: BridgeClient, flag: str
) -> None:
    """Every boolean channel flag can be switched on and off, and reads
    back as 1 or 0 accordingly."""
    tcl_bridge.eval_ok(f'channel set {CHAN} +{flag}')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} {flag}') == "1"
    tcl_bridge.eval_ok(f'channel set {CHAN} -{flag}')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} {flag}') == "0"


def test_multiple_flags_in_one_call(tcl_bridge: BridgeClient) -> None:
    """Several flags can be changed in a single `channel set` call."""
    tcl_bridge.eval_ok(f'channel set {CHAN} +greet +seen -bitch')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} greet') == "1"
    assert tcl_bridge.eval_ok(f'channel get {CHAN} seen') == "1"
    assert tcl_bridge.eval_ok(f'channel get {CHAN} bitch') == "0"


def test_flags_and_values_mixed_in_one_call(tcl_bridge: BridgeClient) -> None:
    """A single call may mix flag toggles with valued settings."""
    tcl_bridge.eval_ok(f'channel set {CHAN} +greet ban-time 99 -bitch')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} greet') == "1"
    assert tcl_bridge.eval_ok(f'channel get {CHAN} ban-time') == "99"
    assert tcl_bridge.eval_ok(f'channel get {CHAN} bitch') == "0"


# ---------- chanmode ----------


def test_chanmode_round_trips(tcl_bridge: BridgeClient) -> None:
    """The enforced channel mode string keeps the modes it was given.

    Order is not preserved -- the modes are rebuilt from a bitmask, so
    `+nt` comes back as `+tn`. Compare as a set of characters, not as a
    string.
    """
    tcl_bridge.eval_ok(f'channel set {CHAN} chanmode {{+nt}}')
    result = tcl_bridge.eval_ok(f'channel get {CHAN} chanmode')
    assert set("nt") <= set(result), result


def test_chanmode_with_argument_round_trips(tcl_bridge: BridgeClient) -> None:
    """A mode taking an argument, such as a user limit, is kept with its
    argument."""
    tcl_bridge.eval_ok(f'channel set {CHAN} chanmode {{+ntl 50}}')
    result = tcl_bridge.eval_ok(f'channel get {CHAN} chanmode')
    assert "l" in result
    assert "50" in result


# ---------- error handling ----------


def test_unknown_setting_is_an_error(tcl_bridge: BridgeClient) -> None:
    """Setting a name that isn't in the table is reported as an error."""
    ok, _ = tcl_bridge.eval(f'channel set {CHAN} no-such-setting 1')
    assert not ok


def test_get_unknown_setting_is_an_error(tcl_bridge: BridgeClient) -> None:
    """Reading a setting that isn't in the table is reported as an error."""
    ok, _ = tcl_bridge.eval(f'channel get {CHAN} no-such-setting')
    assert not ok


def test_setting_on_unknown_channel_is_an_error(
    tcl_bridge: BridgeClient,
) -> None:
    """Setting anything on a channel the bot doesn't have is an error."""
    ok, _ = tcl_bridge.eval('channel set #nosuchchannel ban-time 60')
    assert not ok


def test_missing_value_is_an_error(tcl_bridge: BridgeClient) -> None:
    """A valued setting given no value is rejected rather than defaulting.

    Each branch in tcl_channel_modify checks `i >= items` and returns
    TCL_ERROR with a "needs argument" message.
    """
    ok, result = tcl_bridge.eval(f'channel set {CHAN} ban-time')
    assert not ok
    assert "argument" in result.lower()


# ---------- channel introspection ----------


def test_validchan_distinguishes_known_channels(
    tcl_bridge: BridgeClient,
) -> None:
    """`validchan` reports whether the bot has a record for a channel."""
    assert tcl_bridge.eval_ok(f"validchan {CHAN}") == "1"
    assert tcl_bridge.eval_ok("validchan #nosuchchannel") == "0"


def test_channels_lists_configured_channel(tcl_bridge: BridgeClient) -> None:
    """`channels` returns the channels the bot is configured for."""
    assert CHAN in tcl_bridge.eval_ok("channels")


def test_setchaninfo_and_getchaninfo_round_trip(
    tcl_bridge: BridgeClient,
) -> None:
    """A user's per-channel info line round-trips through set and get.

    Both commands take `handle channel` (tclchan.c:1907) -- this is the
    per-user greeting shown on join, not a description of the channel.
    """
    tcl_bridge.eval_ok("adduser infouser")
    tcl_bridge.eval_ok(f"setchaninfo infouser {CHAN} {{a greeting}}")
    assert tcl_bridge.eval_ok(f"getchaninfo infouser {CHAN}") == "a greeting"


def test_getchaninfo_for_unknown_handle_is_empty(
    tcl_bridge: BridgeClient,
) -> None:
    """Asking for the channel info of a handle that doesn't exist returns
    an empty string rather than an error (tclchan.c:1910)."""
    assert tcl_bridge.eval_ok(f"getchaninfo nosuchuser {CHAN}") == ""


def test_aop_delay_is_a_min_max_pair(tcl_bridge: BridgeClient) -> None:
    """The auto-op delay is a minimum/maximum pair, not a single number.

    Setting it to one value sets both bounds, so `aop-delay 42` reads back
    as "42 42" (chan->aop_min and chan->aop_max, tclchan.c:1177).
    """
    tcl_bridge.eval_ok(f'channel set {CHAN} aop-delay 42')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} aop-delay') == "42 42"


def test_aop_delay_accepts_explicit_range(tcl_bridge: BridgeClient) -> None:
    """Given a range, the auto-op delay keeps both bounds separately."""
    tcl_bridge.eval_ok(f'channel set {CHAN} aop-delay 10:20')
    assert tcl_bridge.eval_ok(f'channel get {CHAN} aop-delay') == "10 20"


def test_flood_setting_output_is_not_valid_input(
    tcl_bridge: BridgeClient,
) -> None:
    """The form a flood limit is reported in cannot be given back to
    `channel set`.

    `channel get` renders the pair as "7 30", but `channel set` accepts
    only the colon form and rejects the space form with "flood value must
    be in X:Y format". So read-modify-write on a flood setting fails
    unless the script converts between the two spellings itself.

    This asymmetry looks like a wart rather than a design decision, but it
    is long-standing behaviour, so it is pinned here rather than fixed.
    """
    tcl_bridge.eval_ok(f'channel set {CHAN} flood-chan 7:30')
    reported = tcl_bridge.eval_ok(f'channel get {CHAN} flood-chan')
    assert reported == "7 30"

    ok, result = tcl_bridge.eval(f'channel set {CHAN} flood-chan {{{reported}}}')
    assert not ok
    assert "X:Y" in result
