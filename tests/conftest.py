"""Pytest fixtures for the Eggdrop integration test harness."""

from __future__ import annotations

import contextlib
import os
from collections.abc import Generator, Iterator
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import pytest
from jinja2 import Environment, FileSystemLoader, StrictUndefined

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.mock_ircd import MockIrcd
from support.waiters import wait_for_file

REPO_ROOT = Path(
    os.environ.get("EGGDROP_SRC", str(Path(__file__).resolve().parents[1]))
)
EGGDROP_BIN = Path(os.environ.get("EGGDROP_BIN", str(REPO_ROOT / "eggdrop")))
TEMPLATES_DIR = Path(__file__).resolve().parent / "templates"
BRIDGE_TCL = Path(__file__).resolve().parent / "support" / "test_bridge.tcl"

_jinja_env = Environment(
    loader=FileSystemLoader(str(TEMPLATES_DIR)),
    undefined=StrictUndefined,
    keep_trailing_newline=True,
)


# ---------- session-wide process tracker ----------


@pytest.fixture(scope="session", autouse=True)
def _process_tracker() -> Iterator[list[EggdropProc]]:
    """Backstop: kill any eggdrop processes a test forgot to clean up."""
    tracked: list[EggdropProc] = []
    yield tracked
    for proc in tracked:
        with contextlib.suppress(Exception):
            proc.terminate(timeout=2)


# ---------- per-test fixtures ----------


@pytest.fixture
def tmp_eggdir(tmp_path: Path) -> Path:
    return tmp_path


@dataclass
class EggdropConfig:
    tmp: Path
    mock_ircd_port: int
    overrides: dict[str, Any] = field(default_factory=dict)
    rendered: bool = False
    config_path: Path = field(init=False)
    userfile_path: Path = field(init=False)
    chanfile_path: Path = field(init=False)
    pidfile_path: Path = field(init=False)
    logfile_path: Path = field(init=False)

    def __post_init__(self) -> None:
        self.config_path = self.tmp / "eggdrop.conf"
        self.userfile_path = self.tmp / "eggdrop.user"
        self.chanfile_path = self.tmp / "eggdrop.chan"
        self.pidfile_path = self.tmp / "eggdrop.pid"
        self.logfile_path = self.tmp / "eggdrop.log"

    def context(self) -> dict[str, Any]:
        ctx: dict[str, Any] = dict(
            nick="TestBot",
            altnick="Test_?",
            realname="pytest bot",
            username="test",
            admin="pytest <test@example.com>",
            network="TestNet",
            botnet_nick="TestBot",
            owner_handle="owner",
            owner_flags="nmto",
            owner_hostmask="*!*@127.0.0.1",
            mod_path=str(REPO_ROOT) + "/",
            help_path=str(REPO_ROOT / "help") + "/",
            bridge_tcl_path=str(BRIDGE_TCL),
            modules=[
                "pbkdf2",
                "channels",
                "server",
                "ctcp",
                "irc",
                "console",
                "notes",
            ],
            extra_modules=[],
            channels=[{"name": "#test", "chanmode": "+nt"}],
            chanfile_channels=[],
            userfile_ban_lines=[],
            userfile_chan_ban_lines={},
            extra_tcl="",
            server_cycle_wait=10,
            server_timeout=30,
            log_flags="mcorvxd",
            tmpdir=str(self.tmp),
            userfile_path=str(self.userfile_path),
            chanfile_path=str(self.chanfile_path),
            pidfile_path=str(self.pidfile_path),
            logfile_path=str(self.logfile_path),
            mock_ircd_port=self.mock_ircd_port,
        )
        ctx.update(self.overrides)
        return ctx

    def render(self, **overrides: Any) -> None:
        self.overrides.update(overrides)
        ctx = self.context()
        self.config_path.write_text(
            _jinja_env.get_template("eggdrop.conf.j2").render(**ctx)
        )
        self.userfile_path.write_text(
            _jinja_env.get_template("userfile.j2").render(**ctx)
        )
        # channels.mod wants to fopen the chanfile read-write; rendering
        # an empty `chanfile_channels` list produces an empty file, same
        # as touching it would.
        self.chanfile_path.write_text(
            _jinja_env.get_template("chanfile.j2").render(**ctx)
        )
        self.rendered = True


@pytest.fixture
def mock_ircd() -> Iterator[MockIrcd]:
    ircd = MockIrcd().start()
    try:
        yield ircd
    finally:
        with contextlib.suppress(Exception):
            ircd.stop()


@pytest.fixture
def eggdrop_config(tmp_eggdir: Path, mock_ircd: MockIrcd) -> EggdropConfig:
    return EggdropConfig(tmp=tmp_eggdir, mock_ircd_port=mock_ircd.port)


def _request_clean_shutdown(port_file: Path) -> None:
    """Open a fresh bridge connection and send Tcl `die` so Eggdrop exits via
    its own atexit path (writes userfile/chanfile, runs gcov atexit, etc.).

    The bridge connection drops as a side effect; that's fine — we just need
    the request to land. All errors are swallowed; this is best-effort.
    """
    if not port_file.exists():
        return
    with contextlib.suppress(Exception):
        port = int(port_file.read_text().strip())
        with BridgeClient("127.0.0.1", port, timeout=2.0) as client:
            client.eval("die test cleanup", timeout=2.0)


@pytest.fixture
def eggdrop_proc(
    eggdrop_config: EggdropConfig,
    tmp_eggdir: Path,
    _process_tracker: list[EggdropProc],
    request: pytest.FixtureRequest,
) -> Iterator[EggdropProc]:
    if not eggdrop_config.rendered:
        eggdrop_config.render()
    if not EGGDROP_BIN.exists():
        pytest.skip(f"eggdrop binary not found at {EGGDROP_BIN}")
    port_file = tmp_eggdir / "bridge.port"
    terminal = request.node.get_closest_marker("partyline") is not None
    proc = EggdropProc(
        binary=EGGDROP_BIN,
        config_path=eggdrop_config.config_path,
        cwd=tmp_eggdir,
        env={
            "EGGDROP_TEST": "1",
            "EGGDROP_TEST_PORT_FILE": str(port_file),
            "EGG_LANGDIR": str(REPO_ROOT / "language"),
        },
        terminal=terminal,
    )
    proc.start()
    _process_tracker.append(proc)
    try:
        yield proc
    finally:
        # Prefer a clean Tcl `die` so atexit handlers (incl. gcov) run; fall
        # back to SIGTERM with a generous timeout for slow CI disks where the
        # gcov .gcda dump on exit can take several seconds.
        _request_clean_shutdown(port_file)
        rc = proc.terminate(timeout=30)
        # On failure, attach the eggdrop log to the report.
        rep = getattr(request.node, "rep_call", None)
        if rep is not None and rep.failed:
            try:
                log = proc.log_path.read_text()
            except OSError:
                log = "(log unavailable)"
            request.node.add_report_section(
                "call",
                "eggdrop stdout",
                f"exit={rc}\nlog at {proc.log_path}\n--- log ---\n{log}",
            )


@pytest.fixture
def tcl_bridge(
    eggdrop_proc: EggdropProc, tmp_eggdir: Path
) -> Iterator[BridgeClient]:
    port_file = tmp_eggdir / "bridge.port"
    try:
        wait_for_file(port_file, timeout=10.0)
    except Exception:
        eggdrop_proc.assert_alive()
        raise
    port = int(port_file.read_text().strip())
    client = BridgeClient.connect_with_retry(
        "127.0.0.1", port, total_timeout=5.0
    )
    try:
        yield client
    finally:
        client.close()


# ---------- failure-report attachment hook ----------


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(
    item: pytest.Item, call: pytest.CallInfo[None]
) -> Generator[None, Any, None]:
    outcome = yield
    rep = outcome.get_result()
    setattr(item, f"rep_{rep.when}", rep)
