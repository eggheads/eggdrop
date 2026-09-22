"""Shared IRC-side test helpers.

These wrap common multi-step interactions with the mock IRCd so individual
tests don't repeat boilerplate. Designed to be importable from any test:

    from support.irc_helpers import (
        drive_registration,
        drive_join_with_names,
        wait_for_isupport,
    )
"""

from __future__ import annotations

import time

from .bridge_client import BridgeClient
from .mock_ircd import MockIrcd, MockIrcdError
from .waiters import wait_for

PREFIX_SYMBOLS = "~&@%+"


def split_member_prefix(token: str) -> tuple[str, str]:
    """Split a NAMES token into (nick, prefix_symbols).

    >>> split_member_prefix("@alice")
    ('alice', '@')
    >>> split_member_prefix("~&boss")
    ('boss', '~&')
    >>> split_member_prefix("plain")
    ('plain', '')
    """
    i = 0
    while i < len(token) and token[i] in PREFIX_SYMBOLS:
        i += 1
    return token[i:], token[:i]


def drive_registration(
    mock_ircd: MockIrcd,
    nick: str = "TestBot",
    isupport_tokens: list[str] | None = None,
) -> None:
    """Drive Eggdrop through IRC registration.

    Waits for the TCP connect, drains the bot's NICK + USER, then sends the
    welcome sequence (001-004 + optional 005 with `isupport_tokens` + 376).
    Returns once the welcome is on the wire.

    To influence which IRCv3 caps the bot negotiates, construct the IRCd with
    `MockIrcd(advertised_caps=[...])` (typically via a local `mock_ircd`
    fixture override). The cap list has to be set at construction time
    because the bot sends `CAP LS 302` immediately on TCP connect — before
    this helper runs.
    """
    mock_ircd.wait_for_connect(timeout=10.0)
    for _ in range(2):  # NICK and USER
        mock_ircd.recv(timeout=5.0)
    mock_ircd.send_welcome(nick=nick, isupport=isupport_tokens)


def drive_join_with_names(
    mock_ircd: MockIrcd,
    members_with_prefix: str,
    nick: str = "TestBot",
    server: str = "mock.test",
    member_accounts: dict[str, str] | None = None,
) -> str:
    """Drive a realistic post-registration channel JOIN to completion.

    Sequence:
      1. wait for the bot's `JOIN #chan`
      2. echo `:nick!u@h JOIN :#chan` back (this populates `chan->name`)
      3. send NAMES (353) with `members_with_prefix` + end (366)
      4. drain the bot's post-join queries:
         * `MODE #chan +b/+e/+I` → empty 368/349/347 end-of-list replies
         * `WHO #chan ...` → if the bot sent a WHOX-style request (the
           `c%chnufat,222` form, used when WHOX ISUPPORT is on), reply with
           one 354 per member carrying the per-member account from
           `member_accounts` (default "*" = not logged in). Otherwise reply
           with one 352 per member. Either form ends with 315.
      5. leave `MODE #chan` (no list flag) unanswered so tests can send
         their own 324 reply

    `member_accounts` maps member nick → account name; nicks not in the dict
    get "*". Only consulted on the WHOX path; ignored for plain WHO.
    Returns the channel name. Quiesces when no new lines arrive for ~300 ms
    or after a 5 s hard cap.
    """
    join_line = mock_ircd.drain_until(
        lambda line: line.startswith("JOIN "), timeout=10.0
    )[-1]
    chan = join_line.split()[1]
    mock_ircd.send(f":{nick}!u@h JOIN :{chan}")
    mock_ircd.send(f":{server} 353 {nick} = {chan} :{members_with_prefix}")
    mock_ircd.send(f":{server} 366 {nick} {chan} :End of /NAMES list.")

    members: list[tuple[str, str]] = [
        split_member_prefix(t) for t in members_with_prefix.split() if t
    ]
    accounts = member_accounts or {}

    def reply_who(whox: bool) -> None:
        for member_nick, prefix_syms in members:
            ident = "u"
            host = "h.example.com"
            flags = "H" + prefix_syms  # H = here (not away)
            if whox:
                # 354 format from chan.c:got354:
                # ":<srv> 354 <botnick> 222 <chan> <user> <host> <nick> <flags> <account>"
                acct = accounts.get(member_nick, "*")
                mock_ircd.send(
                    f":{server} 354 {nick} 222 {chan} {ident} {host} "
                    f"{member_nick} {flags} {acct}"
                )
            else:
                mock_ircd.send(
                    f":{server} 352 {nick} {chan} {ident} {host} {server} "
                    f"{member_nick} {flags} :0 {member_nick}"
                )
        mock_ircd.send(f":{server} 315 {nick} {chan} :End of /WHO list.")

    deadline = time.monotonic() + 5.0
    idle = 0.3
    while time.monotonic() < deadline:
        try:
            line = mock_ircd.recv(timeout=idle)
        except MockIrcdError:
            return chan  # quiesced
        if line.startswith(f"MODE {chan} +b"):
            mock_ircd.send(f":{server} 368 {nick} {chan} :End of Channel Ban List")
        elif line.startswith(f"MODE {chan} +e"):
            mock_ircd.send(
                f":{server} 349 {nick} {chan} :End of Channel Exception List"
            )
        elif line.startswith(f"MODE {chan} +I"):
            mock_ircd.send(
                f":{server} 347 {nick} {chan} :End of Channel Invite List"
            )
        elif line.startswith(f"WHO {chan}"):
            # WHOX form is `WHO #chan c%chnufat,222`; eggdrop emits this when
            # use_354 (WHOX ISUPPORT) is on and parses replies from got354.
            reply_who(whox=",222" in line)
        # MODE #chan (no list flag) — left for the test to answer with 324.
        # Anything else is silently drained.
    return chan


def wait_for_isupport(
    bridge: BridgeClient, key: str, expected: str, timeout: float = 5.0
) -> None:
    """Block until `isupport get <key>` returns `expected`."""
    wait_for(
        lambda: bridge.eval_ok(f"isupport get {key}") == expected,
        timeout=timeout,
        description=f"isupport {key}={expected!r}",
    )
