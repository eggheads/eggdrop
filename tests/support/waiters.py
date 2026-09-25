"""Wait primitives with explicit timeouts. No bare time.sleep loops in tests."""

from __future__ import annotations

import re
import time
from collections.abc import Callable
from pathlib import Path


class WaitTimeout(Exception):
    """Raised by any `wait_for*` helper when its deadline passes."""


def wait_for_file(
    path: Path, timeout: float = 10.0, poll: float = 0.05
) -> None:
    """Block until `path` exists. Raises WaitTimeout."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if path.exists():
            return
        time.sleep(poll)
    raise WaitTimeout(f"file {path} did not appear within {timeout}s")


def wait_for(
    predicate: Callable[[], bool],
    timeout: float = 10.0,
    poll: float = 0.05,
    description: str = "predicate",
) -> None:
    """Poll `predicate()` every `poll` seconds until it returns truthy.

    `description` is included in the timeout error message — supply something
    specific so test failures are self-explanatory.
    """
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(poll)
    raise WaitTimeout(f"{description} not satisfied within {timeout}s")


def wait_for_log_match(
    log_text: Callable[[], str],
    pattern: str,
    timeout: float = 10.0,
    poll: float = 0.05,
) -> re.Match[str]:
    """Block until `pattern` matches anywhere in the log text."""
    rx = re.compile(pattern, re.MULTILINE)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        m = rx.search(log_text())
        if m:
            return m
        time.sleep(poll)
    raise WaitTimeout(
        f"log pattern /{pattern}/ not seen within {timeout}s"
    )
