"""Helpers for building userfile content programmatically.

Tests inject pre-populated ban records into the userfile via the
`userfile_ban_lines` (global) and `userfile_chan_ban_lines` (per-channel)
context variables of `templates/userfile.j2`. The template just emits
the strings verbatim — all formatting and escaping happens here so the
template stays a flat iteration.
"""

from __future__ import annotations


def format_userfile_ban(
    *,
    mask: str,
    perm: bool,
    sticky: bool,
    expire: int,
    added: int,
    lastactive: int,
    creator: str,
    desc: str,
) -> str:
    """Format one ban record for the userfile.

    Mirrors the line written by `write_bans` in
    `src/mod/channels.mod/userchan.c`:

        - <mask>:<perm-prefix><expire><sticky-suffix>:+<added>:<lastactive>:<creator>:<desc>

    `:` and `\\` in the mask are hex-escaped per `src/misc.c:str_escape`
    (the parser uses `\\xy` as a hex byte; a literal `\\:` would yield NUL
    via strtol of ":?" base 16 and silently truncate the mask). All
    arguments are required — no defaults — so each test states exactly
    what it's pinning.
    """
    escaped = mask.replace("\\", "\\5c").replace(":", "\\3a")
    perm_prefix = "+" if perm else ""
    sticky_suffix = "*" if sticky else ""
    return (
        f"- {escaped}:{perm_prefix}{expire}{sticky_suffix}:"
        f"+{added}:{lastactive}:{creator}:{desc}"
    )
