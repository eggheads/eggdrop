"""Minimal mock IRCd for Eggdrop integration tests.

asyncio TCP server hosted on its own thread + private event loop.
Tests interact via a synchronous facade: `recv()`, `send()`, `expect_recv_match()`,
`send_welcome()`. The mock auto-replies to PING; everything else lands in a
queue for tests to consume and assert on.

Default behaviour is one client connection per `MockIrcd` instance. Tests that
exercise reconnect logic should pass `allow_reconnect=True`.
"""

from __future__ import annotations

import asyncio
import contextlib
import re
import threading
import time
from collections.abc import Callable, Coroutine, Iterable
from concurrent.futures import Future
from typing import Any, TypeVar

_T = TypeVar("_T")


class MockIrcdError(Exception):
    """Raised on a timeout or other harness-side error against the mock IRCd."""


class UnexpectedReconnect(MockIrcdError):
    """Raised at `stop()` if the bot reconnected when `allow_reconnect=False`."""


class MockIrcd:
    """Synchronous-facade mock IRCd, listening on `127.0.0.1:0`.

    Internally an asyncio TCP server runs on a private event loop in a
    background thread. The public API is plain blocking calls (`recv()`,
    `send()`, `expect_recv_match()`, ...) so test code stays linear.

    Auto-handles client `PING` and (by default) the `CAP LS`/`REQ`/`END`
    handshake so individual tests don't have to repeat that boilerplate.
    Pass `auto_cap=False` to drive CAP negotiation explicitly.
    """

    def __init__(
        self,
        allow_reconnect: bool = False,
        auto_cap: bool = True,
        server_name: str = "mock.test",
        advertised_caps: Iterable[str] | None = None,
    ) -> None:
        """Configure (but do not start) a mock IRCd.

        `allow_reconnect`: if False, a second client connection during the
            test is treated as a hard error at `stop()` time.
        `auto_cap`: auto-respond to `CAP LS`/`REQ`/`LIST`. `CAP REQ` is
            always ACKed for whatever the bot asks for.
        `server_name`: source prefix for synthetic numerics (`:server 001 ...`).
        `advertised_caps`: caps offered in `CAP LS` replies. Default empty.
            Tests that need specific caps construct their own MockIrcd
            (typically via a local `mock_ircd` fixture override) — by the
            time the test body runs the bot has already sent `CAP LS 302`,
            so the cap list must be set at construction time.
        """
        self._allow_reconnect = allow_reconnect
        self._auto_cap = auto_cap
        self._server_name = server_name
        self._advertised_caps = " ".join(advertised_caps or [])
        self._loop = asyncio.new_event_loop()
        self._thread = threading.Thread(
            target=self._loop.run_forever, name="MockIrcd", daemon=True
        )
        self._server: asyncio.base_events.Server | None = None
        self._writer: asyncio.StreamWriter | None = None
        self._recv_q: asyncio.Queue[str] | None = None
        self._connect_evt: asyncio.Event | None = None
        self._connect_count = 0
        self._unexpected_reconnect = False
        self.port: int = 0

    # ---------- lifecycle ----------

    def start(self) -> MockIrcd:
        """Start the asyncio loop thread and bind the listener. Sets `self.port`."""
        self._thread.start()
        self._submit(self._async_start()).result(timeout=5)
        return self

    def stop(self) -> None:
        """Close the listener, stop the loop thread, and assert no rogue reconnects.

        Raises `UnexpectedReconnect` if the client connected more than once
        without `allow_reconnect=True`.
        """
        if self._loop.is_running():
            with contextlib.suppress(Exception):
                self._submit(self._async_stop()).result(timeout=5)
            self._loop.call_soon_threadsafe(self._loop.stop)
        self._thread.join(timeout=5)
        if self._unexpected_reconnect and not self._allow_reconnect:
            raise UnexpectedReconnect(
                f"client connected {self._connect_count} times "
                f"(allow_reconnect=False)"
            )

    def __enter__(self) -> MockIrcd:
        return self.start()

    def __exit__(self, *_: object) -> None:
        self.stop()

    async def _async_start(self) -> None:
        self._recv_q = asyncio.Queue()
        self._connect_evt = asyncio.Event()
        self._server = await asyncio.start_server(
            self._on_client, "127.0.0.1", 0
        )
        self.port = self._server.sockets[0].getsockname()[1]

    async def _async_stop(self) -> None:
        if self._writer is not None:
            self._writer.close()
            # wait_closed can hang if the peer is gone (FIN never ACKed).
            # Bound it tightly — we're tearing down the loop anyway.
            with contextlib.suppress(Exception):
                await asyncio.wait_for(self._writer.wait_closed(), timeout=0.5)
        if self._server is not None:
            self._server.close()
            with contextlib.suppress(Exception):
                await asyncio.wait_for(self._server.wait_closed(), timeout=0.5)

    # ---------- client handler ----------

    async def _on_client(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        self._connect_count += 1
        if self._connect_count > 1 and not self._allow_reconnect:
            self._unexpected_reconnect = True
            writer.close()
            return
        self._writer = writer
        assert self._connect_evt is not None
        self._connect_evt.set()
        try:
            while True:
                line = await reader.readline()
                if not line:
                    break
                text = line.rstrip(b"\r\n").decode("utf-8", "replace")
                upper = text.upper()
                if upper.startswith("PING"):
                    pong = "PONG" + text[4:] + "\r\n"
                    writer.write(pong.encode())
                    await writer.drain()
                elif self._auto_cap and upper.startswith("CAP "):
                    self._handle_cap(text, writer)
                    await writer.drain()
                else:
                    assert self._recv_q is not None
                    await self._recv_q.put(text)
        finally:
            self._writer = None

    def _handle_cap(self, text: str, writer: asyncio.StreamWriter) -> None:
        """Auto-respond to client CAP commands so tests don't have to.

        Replies to `CAP LS` with the caps the IRCd was constructed with
        (default empty), and ACKs whatever the bot then asks for in
        `CAP REQ`.
        """
        parts = text.split(maxsplit=2)
        sub = parts[1].upper() if len(parts) >= 2 else ""
        srv = self._server_name
        # Use "*" as the unregistered nick placeholder per RFC.
        if sub == "LS":
            writer.write(f":{srv} CAP * LS :{self._advertised_caps}\r\n".encode())
        elif sub == "REQ":
            cap = parts[2] if len(parts) >= 3 else ":"
            if cap.startswith(":"):
                cap = cap[1:]
            writer.write(f":{srv} CAP * ACK :{cap}\r\n".encode())
        elif sub == "END":
            pass  # no response needed
        elif sub == "LIST":
            writer.write(f":{srv} CAP * LIST :\r\n".encode())

    # ---------- synchronous facade ----------

    def _submit(self, coro: Coroutine[Any, Any, _T]) -> Future[_T]:
        """Schedule a coroutine on the bg loop, return a thread-safe Future."""
        return asyncio.run_coroutine_threadsafe(coro, self._loop)

    def wait_for_connect(self, timeout: float = 10.0) -> None:
        """Block until the first client (Eggdrop) opens the TCP connection."""
        async def _wait() -> None:
            assert self._connect_evt is not None
            await asyncio.wait_for(self._connect_evt.wait(), timeout)

        self._submit(_wait()).result(timeout=timeout + 1)

    def recv(self, timeout: float = 5.0) -> str:
        """Pop the next non-PING/CAP line from the client's recv queue.

        Raises `MockIrcdError` if no line arrives within `timeout`.
        """
        async def _recv() -> str:
            assert self._recv_q is not None
            return await asyncio.wait_for(self._recv_q.get(), timeout)

        try:
            return self._submit(_recv()).result(timeout=timeout + 1)
        except TimeoutError as e:
            raise MockIrcdError(
                f"no IRC line received within {timeout}s"
            ) from e

    def expect_recv(self, expected: str, timeout: float = 5.0) -> str:
        """`recv()` and assert the line equals `expected` exactly."""
        line = self.recv(timeout)
        if line != expected:
            raise AssertionError(
                f"expected IRC line {expected!r}, got {line!r}"
            )
        return line

    def expect_recv_match(self, pattern: str, timeout: float = 5.0) -> re.Match[str]:
        """`recv()` and assert the line matches `pattern` (re.match anchored)."""
        line = self.recv(timeout)
        m = re.match(pattern, line)
        if not m:
            raise AssertionError(
                f"expected IRC line matching /{pattern}/, got {line!r}"
            )
        return m

    def drain_until(
        self, predicate: Callable[[str], bool], timeout: float = 5.0
    ) -> list[str]:
        """Read lines until `predicate(line)` is True. Returns the lines read
        (including the matching one). Useful for skipping registration noise."""
        deadline = time.monotonic() + timeout
        seen: list[str] = []
        while time.monotonic() < deadline:
            remaining = max(0.05, deadline - time.monotonic())
            line = self.recv(remaining)
            seen.append(line)
            if predicate(line):
                return seen
        raise MockIrcdError(
            f"predicate not satisfied within {timeout}s; saw {len(seen)} lines"
        )

    def send(self, line: str) -> None:
        """Push a single IRC line (CRLF appended automatically) to the client."""
        async def _send() -> None:
            assert self._writer is not None, "no client connected"
            data = (line.rstrip("\r\n") + "\r\n").encode()
            self._writer.write(data)
            await self._writer.drain()

        self._submit(_send()).result(timeout=5)

    def send_from(self, prefix: str, rest: str) -> None:
        """Convenience: `send(":<prefix> <rest>")`. Use for messages from other users."""
        self.send(f":{prefix} {rest}")

    def send_welcome(
        self,
        nick: str,
        server: str = "mock.test",
        isupport: Iterable[str] | None = None,
    ) -> None:
        """Send the registration response: 001-004, optional 005, then 376.

        `isupport` is a list of raw `KEY=VALUE` (or bare `KEY`) tokens
        joined into the 005 line; pass `None` to skip the 005 entirely.
        """
        self.send(f":{server} 001 {nick} :Welcome to mock {nick}")
        self.send(f":{server} 002 {nick} :Your host is {server}")
        self.send(f":{server} 003 {nick} :This server was created today")
        self.send(f":{server} 004 {nick} {server} mock-1.0 oiwsx ovimnpst")
        if isupport:
            tokens = " ".join(isupport)
            self.send(f":{server} 005 {nick} {tokens} :are supported")
        self.send(f":{server} 376 {nick} :End of MOTD")

    def send_names(
        self, nick: str, channel: str, members: Iterable[str], server: str = "mock.test"
    ) -> None:
        """Send a 353 NAMES line followed by the 366 end-of-list."""
        names = " ".join(members)
        self.send(f":{server} 353 {nick} = {channel} :{names}")
        self.send(f":{server} 366 {nick} {channel} :End of /NAMES")
