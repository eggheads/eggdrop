"""Wildcard and hostmask matching (src/match.c).

Driven through the Tcl `matchstr` / `matchaddr` / `matchcidr` commands, which
are thin wrappers over `wild_match` (`_wild_match`), `match_addr`
(`addr_match`) and `cidr_match` respectively — see src/tclmisc.c:734-765.

These are *characterization* tests: they pin down what match.c currently
does, quirks included, so the file can be refactored safely. Where the
current behaviour looks surprising the docstring says so rather than
asserting what would be "nicer".

Two things worth knowing before reading the cases:

- `wild_match` understands only `*` and `?`. The `%` (non-space) and `~`
  (whitespace) wildcards live in `_wild_match_per`, which `matchstr` does
  not call, so those characters are literals here.
- `_wild_match` rejects an empty pattern or an empty subject outright
  (src/match.c:161-163) before looking at wildcards at all. That makes
  `matchstr * ""` return 0, which surprises most people.

Tcl quoting note: every argument is wrapped in braces so the interpreter
performs no substitution. Patterns containing `$`, `[` or `\\` would
otherwise be mangled before Eggdrop ever sees them.
"""

from __future__ import annotations

import pytest

from support.bridge_client import BridgeClient


def matchstr(bridge: BridgeClient, pattern: str, subject: str) -> str:
    """Evaluate `matchstr <pattern> <subject>` with substitution suppressed."""
    return bridge.eval_ok(f"matchstr {{{pattern}}} {{{subject}}}")


# ---------- literal matching ----------


@pytest.mark.parametrize(
    ("pattern", "subject", "expected"),
    [
        ("abc", "abc", "1"),
        ("abc", "abd", "0"),
        ("abc", "ab", "0"),
        ("ab", "abc", "0"),
    ],
)
def test_matchstr_literal(
    tcl_bridge: BridgeClient, pattern: str, subject: str, expected: str
) -> None:
    """A pattern with no wildcards matches only the identical string."""
    assert matchstr(tcl_bridge, pattern, subject) == expected


@pytest.mark.parametrize(
    ("pattern", "subject"),
    [
        ("ABC", "abc"),
        ("abc", "ABC"),
        ("AbC", "aBc"),
        ("*ABC", "xxxabc"),
    ],
)
def test_matchstr_is_case_insensitive(
    tcl_bridge: BridgeClient, pattern: str, subject: str
) -> None:
    """Matching ignores case, via toupper() on both sides."""
    assert matchstr(tcl_bridge, pattern, subject) == "1"


# ---------- '*' — zero or more of anything ----------


@pytest.mark.parametrize(
    ("pattern", "subject", "expected"),
    [
        ("*", "anything", "1"),
        ("*", "a", "1"),
        ("a*", "abc", "1"),
        ("a*", "a", "1"),          # '*' may match zero characters
        ("*c", "abc", "1"),
        ("*c", "c", "1"),
        ("a*c", "abc", "1"),
        ("a*c", "ac", "1"),
        ("a*c", "abbbbc", "1"),
        ("a*c", "abcd", "0"),
        ("*b*", "abc", "1"),
        ("*x*", "abc", "0"),
        ("*", "a b c", "1"),       # '*' spans spaces
        ("a*c", "a b c", "1"),
    ],
)
def test_matchstr_star(
    tcl_bridge: BridgeClient, pattern: str, subject: str, expected: str
) -> None:
    """`*` matches zero or more characters, including spaces."""
    assert matchstr(tcl_bridge, pattern, subject) == expected


@pytest.mark.parametrize("pattern", ["**", "***", "a**c", "*a*", "**a**"])
def test_matchstr_consecutive_stars_collapse(
    tcl_bridge: BridgeClient, pattern: str
) -> None:
    """Runs of `*` behave as a single `*`.

    The "Zap redundant wilds" loop at src/match.c:186-188 walks back over
    consecutive WILDS, so no amount of repetition changes the result. This
    also guards the endless-loop bug noted in the file header.
    """
    subject = "abc" if "a" in pattern.replace("*", "") else "abc"
    assert matchstr(tcl_bridge, pattern, subject) == "1"


# ---------- '?' — exactly one character ----------


@pytest.mark.parametrize(
    ("pattern", "subject", "expected"),
    [
        ("?", "a", "1"),
        ("?", "ab", "0"),
        ("?", "", "0"),
        ("a?c", "abc", "1"),
        ("a?c", "ac", "0"),        # '?' will not match zero characters
        ("a?c", "abbc", "0"),
        ("???", "abc", "1"),
        ("???", "ab", "0"),
        ("a?c", "a c", "1"),       # '?' matches a space
    ],
)
def test_matchstr_question(
    tcl_bridge: BridgeClient, pattern: str, subject: str, expected: str
) -> None:
    """`?` matches exactly one character, space included."""
    assert matchstr(tcl_bridge, pattern, subject) == expected


# ---------- empty-string handling (the surprising part) ----------


@pytest.mark.parametrize(
    ("pattern", "subject"),
    [
        ("", ""),
        ("", "abc"),
        ("*", ""),
        ("?", ""),
        ("abc", ""),
    ],
)
def test_matchstr_empty_never_matches(
    tcl_bridge: BridgeClient, pattern: str, subject: str
) -> None:
    """An empty pattern or an empty subject never matches — not even `*`.

    src/match.c:161-163 returns NOMATCH when either string is NULL or
    empty, before any wildcard handling. So `matchstr * ""` is 0 even
    though `*` is documented as matching zero or more characters. This is
    long-standing behaviour; the test pins it rather than endorsing it.
    """
    assert matchstr(tcl_bridge, pattern, subject) == "0"


# ---------- characters that are NOT wildcards here ----------


@pytest.mark.parametrize(
    ("pattern", "subject", "expected"),
    [
        ("%", "%", "1"),           # literal, not the non-space wildcard
        ("%", "abc", "0"),
        ("a%c", "a%c", "1"),
        ("a%c", "abc", "0"),
        ("~", "~", "1"),           # literal, not the whitespace wildcard
        ("~", "   ", "0"),
        ("a~c", "a~c", "1"),
        ("a~c", "a c", "0"),
    ],
)
def test_matchstr_percent_and_tilde_are_literal(
    tcl_bridge: BridgeClient, pattern: str, subject: str, expected: str
) -> None:
    """`%` and `~` are ordinary characters to `matchstr`.

    They are wildcards only in `_wild_match_per`, which handles binds and
    is not reachable from `matchstr`. Anyone assuming `%` works like `*`
    here will be wrong, which is exactly why this is pinned.
    """
    assert matchstr(tcl_bridge, pattern, subject) == expected


# ---------- hostmask matching (match_addr) ----------


@pytest.mark.parametrize(
    ("mask", "address", "expected"),
    [
        ("*!*@*", "nick!user@host.example", "1"),
        ("nick!*@*", "nick!user@host.example", "1"),
        ("NICK!*@*", "nick!user@host.example", "1"),
        ("other!*@*", "nick!user@host.example", "0"),
        ("*!user@*", "nick!user@host.example", "1"),
        ("*!*@host.example", "nick!user@host.example", "1"),
        ("*!*@*.example", "nick!user@host.example", "1"),
        ("*!*@*.other", "nick!user@host.example", "0"),
    ],
)
def test_matchaddr_hostmask(
    tcl_bridge: BridgeClient, mask: str, address: str, expected: str
) -> None:
    """A nick!user@host mask matches an address, case-insensitively."""
    assert tcl_bridge.eval_ok(f"matchaddr {{{mask}}} {{{address}}}") == expected


@pytest.mark.parametrize(
    ("block", "address", "prefix", "expected"),
    [
        ("192.168.1.0", "192.168.1.42", "24", "1"),
        ("192.168.1.0", "192.168.2.42", "24", "0"),
        ("192.168.0.0", "192.168.2.42", "16", "1"),
        ("10.0.0.1", "10.0.0.1", "32", "1"),
        ("10.0.0.1", "10.0.0.2", "32", "0"),
    ],
)
def test_matchcidr_ipv4(
    tcl_bridge: BridgeClient, block: str, address: str, prefix: str, expected: str
) -> None:
    """An IPv4 address is tested against a CIDR block at a given prefix length."""
    assert (
        tcl_bridge.eval_ok(f"matchcidr {{{block}}} {{{address}}} {prefix}") == expected
    )
