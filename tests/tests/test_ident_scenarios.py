"""Ident lookup scenarios for inbound DCC/telnet connections.

Exercises `dcc_telnet_hostresolved2` (src/dcc.c) which is responsible for
performing an RFC 1413 ident query back to the originator of an inbound
DCC connection. Three real-world outcomes are covered:

  * "refused"  — nothing listening on the ident port; kernel RSTs the
                 SYN. `connect_nonblock` returns -4 and the host falls
                 back to `telnet@<host>`.
  * "timeout"  — ident port accepts but never writes a reply. After
                 `ident-timeout` seconds, `timeout_dcc_ident` fires and
                 the host falls back to `telnet@<host>`.
  * "respond"  — ident port replies with a valid `USERID : UNIX : <user>`
                 line; the host is set to `<user>@<host>`.

Each scenario is run twice — once with `ident-timeout 0` (eggdrop skips
the lookup entirely and always falls back to `telnet@<host>`) and once
with `ident-timeout 3` (eggdrop actually does the lookup).

The bot connects to TCP/1113 instead of the wire-standard 113 when
`EGGDROP_TEST` is set in the env (see src/dcc.c:dcc_telnet_hostresolved2)
so the test process doesn't need privileged-port access.
"""

from __future__ import annotations

import contextlib
import re
import socket
from collections.abc import Iterator
from typing import Any

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc  # noqa: F401  (fixture name)
from support.identd import IdentServer
from support.waiters import wait_for

# Owner record matches whatever reverse-DNS returns for 127.0.0.1
# (`localhost`, `127.0.0.1`, or anything in /etc/hosts) so `protect-telnet`
# accepts the inbound connection regardless of the resolver. The user has
# 'n' flag (owner), which implies USER_OP for the `protect_telnet` check
# in dcc_telnet_got_ident.
OWNER_HOSTMASK = "*!*@*"


def _pick_free_port() -> int:
    """Bind to :0, read the assigned port, release. Best-effort — race window
    between close and the eggdrop child binding the same port, but on Linux
    the kernel doesn't immediately reissue closed ports so this is fine."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


@contextlib.contextmanager
def _telnet_client(host: str, port: int) -> Iterator[socket.socket]:
    """Open a TCP connection to the bot's listen port and keep it open for
    the lifetime of the `with` block. The dcc entry lives only as long as
    the socket — closing it ends the test scenario from the bot's side."""
    s = socket.create_connection((host, port), timeout=3.0)
    try:
        yield s
    finally:
        with contextlib.suppress(OSError):
            s.shutdown(socket.SHUT_RDWR)
        s.close()


def _wait_for_telnet_id_host(bridge: BridgeClient, timeout: float) -> str:
    """Poll `dcclist TELNET_ID` until one entry exists; return its host.

    The dcc entry transitions DCC_TELNET → DCC_DNSWAIT → DCC_IDENTWAIT →
    DCC_TELNET_ID once host resolution + ident finish. We watch the final
    state. The host field is `<userpart>@<resolved-host>` per
    `dcc_telnet_got_ident` (src/dcc.c)."""
    cmd = (
        "if {[llength [set l [dcclist TELNET_ID]]]} "
        "{lindex [lindex $l 0] 2}"
    )
    host: dict[str, str] = {}

    def _check() -> bool:
        result = bridge.eval_ok(cmd)
        if result:
            host["v"] = result
            return True
        return False

    wait_for(_check, timeout=timeout, description="dcc TELNET_ID entry to appear")
    return host["v"]


# ---------- ident-timeout 0: lookup is skipped, host always telnet@... ----------


def test_ident_disabled_with_no_identd(
    eggdrop_config: Any,
    request: pytest.FixtureRequest,
) -> None:
    """ident-timeout=0 + no ident responder → host is `telnet@<localhost>`.

    Verifies the early-return path in dcc_telnet_hostresolved2: identtimeout
    is checked *before* the connect to 1113, so the identd presence is
    irrelevant. The userhost should be the fallback `telnet@...` form.
    """
    listen_port = _pick_free_port()
    eggdrop_config.render(
        owner_hostmask=OWNER_HOSTMASK,
        extra_tcl=f"set ident-timeout 0\nlisten {listen_port} all\n",
    )
    bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    with _telnet_client("127.0.0.1", listen_port):
        host = _wait_for_telnet_id_host(bridge, timeout=5.0)

    assert re.fullmatch(r"telnet@(127\.0\.0\.1|localhost|localhost\.\S+)", host), (
        f"expected telnet@<localhost>, got {host!r}"
    )


def test_ident_disabled_with_responder_running(
    eggdrop_config: Any,
    request: pytest.FixtureRequest,
) -> None:
    """ident-timeout=0 + identd running → identd is NOT contacted; host stays
    `telnet@<localhost>`. Confirms the skip path doesn't accidentally fire
    the lookup.
    """
    listen_port = _pick_free_port()
    eggdrop_config.render(
        owner_hostmask=OWNER_HOSTMASK,
        extra_tcl=f"set ident-timeout 0\nlisten {listen_port} all\n",
    )
    bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    with (
        IdentServer("respond", user="alice"),
        _telnet_client("127.0.0.1", listen_port),
    ):
        host = _wait_for_telnet_id_host(bridge, timeout=5.0)

    assert re.fullmatch(r"telnet@(127\.0\.0\.1|localhost|localhost\.\S+)", host), (
        f"expected telnet@<localhost> (identd ignored), got {host!r}"
    )


def test_ident_disabled_with_hung_responder(
    eggdrop_config: Any,
    request: pytest.FixtureRequest,
) -> None:
    """ident-timeout=0 + identd that hangs → host is `telnet@<localhost>`
    immediately (no waiting on the hung server)."""
    listen_port = _pick_free_port()
    eggdrop_config.render(
        owner_hostmask=OWNER_HOSTMASK,
        extra_tcl=f"set ident-timeout 0\nlisten {listen_port} all\n",
    )
    bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    with (
        IdentServer("timeout"),
        _telnet_client("127.0.0.1", listen_port),
    ):
        # Wait window deliberately tight: with ident-timeout=0 the
        # lookup is skipped, so the entry should appear in <1s.
        host = _wait_for_telnet_id_host(bridge, timeout=2.0)

    assert re.fullmatch(r"telnet@(127\.0\.0\.1|localhost|localhost\.\S+)", host), (
        f"expected telnet@<localhost> (identd ignored), got {host!r}"
    )


# ---------- ident-timeout 3: lookup actually runs ----------


def test_ident_lookup_refused(
    eggdrop_config: Any,
    request: pytest.FixtureRequest,
) -> None:
    """ident-timeout=3 + nothing on TCP/1113 → kernel RST → host falls back
    to `telnet@<localhost>`. connect_nonblock detects ECONNREFUSED via its
    own 500ms select, so this resolves quickly regardless of ident-timeout."""
    listen_port = _pick_free_port()
    eggdrop_config.render(
        owner_hostmask=OWNER_HOSTMASK,
        extra_tcl=f"set ident-timeout 3\nlisten {listen_port} all\n",
    )
    bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    with _telnet_client("127.0.0.1", listen_port):
        host = _wait_for_telnet_id_host(bridge, timeout=5.0)

    assert re.fullmatch(r"telnet@(127\.0\.0\.1|localhost|localhost\.\S+)", host), (
        f"expected telnet@<localhost> (ident refused), got {host!r}"
    )


@pytest.mark.slow
def test_ident_lookup_timeout(
    eggdrop_config: Any,
    request: pytest.FixtureRequest,
) -> None:
    """ident-timeout=3 + identd accepts but never replies → DCC_IDENT timeout
    eventually fires; host falls back to `telnet@<localhost>`.

    This is the case that *cannot* be exercised by leaving 1113 unbound —
    we need the TCP handshake to succeed so the dcc enters DCC_IDENT state
    and the app-level identtimeout actually counts down.

    Eggdrop's `check_expired_dcc` only runs every 10s (src/main.c:556) so
    the effective timeout latency is `identtimeout + (up to 10s)`. The 15s
    wait below covers the worst case.
    """
    listen_port = _pick_free_port()
    eggdrop_config.render(
        owner_hostmask=OWNER_HOSTMASK,
        extra_tcl=f"set ident-timeout 3\nlisten {listen_port} all\n",
    )
    bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    with (
        IdentServer("timeout"),
        _telnet_client("127.0.0.1", listen_port),
    ):
        host = _wait_for_telnet_id_host(bridge, timeout=15.0)

    assert re.fullmatch(r"telnet@(127\.0\.0\.1|localhost|localhost\.\S+)", host), (
        f"expected telnet@<localhost> (ident timed out), got {host!r}"
    )


def test_ident_lookup_success(
    eggdrop_config: Any,
    request: pytest.FixtureRequest,
) -> None:
    """ident-timeout=3 + identd replies `USERID : UNIX : alice` → host is
    `alice@<localhost>`. Verifies the full happy-path parse in `dcc_ident`."""
    listen_port = _pick_free_port()
    eggdrop_config.render(
        owner_hostmask=OWNER_HOSTMASK,
        extra_tcl=f"set ident-timeout 3\nlisten {listen_port} all\n",
    )
    bridge: BridgeClient = request.getfixturevalue("tcl_bridge")

    with (
        IdentServer("respond", user="alice"),
        _telnet_client("127.0.0.1", listen_port),
    ):
        host = _wait_for_telnet_id_host(bridge, timeout=5.0)

    assert re.fullmatch(r"alice@(127\.0\.0\.1|localhost|localhost\.\S+)", host), (
        f"expected alice@<localhost>, got {host!r}"
    )
