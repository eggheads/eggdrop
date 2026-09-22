"""Subprocess wrapper for a spawned Eggdrop instance."""

from __future__ import annotations

import contextlib
import io
import os
import re
import signal
import subprocess
import threading
from pathlib import Path
from typing import IO

from .waiters import WaitTimeout, wait_for_log_match


class EggdropDiedError(Exception):
    """Raised when an operation is attempted on an Eggdrop that has exited."""


class EggdropProc:
    """Spawned Eggdrop subprocess with a captured stdout drain.

    `start()` spawns the bot; `terminate()` ends it (SIGTERM, then SIGKILL
    after the grace period). Stdout/stderr are streamed into both an
    in-memory ring (`stdout_text()`) and an on-disk log (`log_path`) so
    failures can be inspected post-mortem.
    """

    def __init__(
        self,
        binary: Path,
        config_path: Path,
        cwd: Path,
        env: dict[str, str] | None = None,
        log_path: Path | None = None,
        terminal: bool = False,
    ) -> None:
        """Configure (but do not start) a wrapped Eggdrop process.

        `terminal=True` spawns with `-nt` (HQ partyline on stdin, owner
        perms auto-granted); `False` uses `-n` (foreground only, no
        partyline). `env` is merged on top of the parent process env.
        """
        self._binary = binary
        self._config = config_path
        self._cwd = cwd
        self._env = {**os.environ, **(env or {})}
        self._log_path = log_path or (cwd / "eggdrop.stdout.log")
        self._terminal = terminal
        self._buf = io.StringIO()
        self._buf_lock = threading.Lock()
        self._proc: subprocess.Popen[bytes] | None = None
        self._drain_thread: threading.Thread | None = None
        self._log_fp: IO[str] | None = None

    def start(self) -> EggdropProc:
        """Spawn Eggdrop and start the background stdout-drain thread."""
        # Lifetime spans process; closed in terminate().
        self._log_fp = open(self._log_path, "w", encoding="utf-8")  # noqa: SIM115
        flags = "-nt" if self._terminal else "-n"
        self._proc = subprocess.Popen(
            [str(self._binary), flags, str(self._config)],
            cwd=str(self._cwd),
            env=self._env,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            bufsize=0,
        )
        self._drain_thread = threading.Thread(
            target=self._drain, name="eggdrop-stdout-drain", daemon=True
        )
        self._drain_thread.start()
        return self

    def _drain(self) -> None:
        # Bind to locals so the iter() closure has non-Optional types.
        proc = self._proc
        assert proc is not None
        assert proc.stdout is not None
        stdout: IO[bytes] = proc.stdout
        log_fp = self._log_fp
        for chunk in iter(lambda: stdout.read(4096), b""):
            text = chunk.decode("utf-8", "replace")
            with self._buf_lock:
                self._buf.write(text)
            if log_fp is not None:
                log_fp.write(text)
                log_fp.flush()

    @property
    def proc(self) -> subprocess.Popen[bytes]:
        assert self._proc is not None, "eggdrop not started"
        return self._proc

    @property
    def pid(self) -> int:
        return self.proc.pid

    @property
    def returncode(self) -> int | None:
        return self.proc.poll()

    @property
    def log_path(self) -> Path:
        return self._log_path

    def stdout_text(self) -> str:
        """Snapshot of everything written to stdout/stderr so far. Thread-safe."""
        with self._buf_lock:
            return self._buf.getvalue()

    def assert_alive(self) -> None:
        """Raise `EggdropDiedError` if the process has exited."""
        rc = self.proc.poll()
        if rc is not None:
            raise EggdropDiedError(
                f"eggdrop exited with rc={rc}\n--- log ---\n{self.stdout_text()}"
            )

    def wait_for_log(self, pattern: str, timeout: float = 10.0) -> re.Match[str]:
        """Poll stdout for `pattern`. If it never appears, surface a useful error.

        On timeout, `assert_alive()` is called first so a dead Eggdrop
        produces an `EggdropDiedError` (with the captured log) rather than
        a generic `WaitTimeout`.
        """
        try:
            return wait_for_log_match(self.stdout_text, pattern, timeout=timeout)
        except WaitTimeout:
            self.assert_alive()
            raise

    def send_partyline(self, line: str) -> None:
        """Write a line to Eggdrop's stdin (only useful with -t/HQ partyline)."""
        proc = self._proc
        assert proc is not None
        assert proc.stdin is not None
        proc.stdin.write((line.rstrip("\n") + "\n").encode("utf-8"))
        proc.stdin.flush()

    def terminate(self, timeout: float = 5.0) -> int:
        """SIGTERM, then SIGKILL after timeout. Returns exit code."""
        proc = self._proc
        if proc is None:
            return 0
        if proc.poll() is None:
            with contextlib.suppress(ProcessLookupError):
                proc.send_signal(signal.SIGTERM)
            try:
                proc.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=2)
        if self._drain_thread is not None:
            self._drain_thread.join(timeout=2)
        if self._log_fp is not None:
            self._log_fp.close()
            self._log_fp = None
        return proc.returncode
