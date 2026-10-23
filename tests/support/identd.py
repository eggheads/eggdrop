"""Minimal ident (RFC 1413) responder for Eggdrop integration tests.

Listens on `127.0.0.1:1113` to match the `EGGDROP_TEST` override in
`src/dcc.c:dcc_telnet_hostresolved2` — when that env var is set, eggdrop
connects to TCP/1113 (unprivileged) instead of the wire-standard 113.

Two modes:

    "respond" — accept, read the bot's request line, reply with
                `<lport>, <rport> : USERID : UNIX : <user>\\r\\n`.
    "timeout" — accept, never write; eggdrop's `identtimeout` fires.

The third ident scenario, "connection refused", is exercised by simply
*not* constructing an `IdentServer` so port 1113 stays unbound and the
kernel returns RST.
"""

from __future__ import annotations

import contextlib
import socket
import threading
from typing import Literal


class IdentServer:
    """Threaded ident responder bound to 127.0.0.1:<port> (default 1113)."""

    def __init__(
        self,
        mode: Literal["respond", "timeout"],
        user: str = "alice",
        host: str = "127.0.0.1",
        port: int = 1113,
    ) -> None:
        self._mode = mode
        self._user = user
        self._host = host
        self._port = port
        self._sock: socket.socket | None = None
        self._thread: threading.Thread | None = None
        self._stop = threading.Event()

    def start(self) -> IdentServer:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((self._host, self._port))
        s.listen(8)
        s.settimeout(0.2)
        self._sock = s
        self._thread = threading.Thread(
            target=self._serve, name=f"identd-{self._mode}", daemon=True
        )
        self._thread.start()
        return self

    def _serve(self) -> None:
        assert self._sock is not None
        while not self._stop.is_set():
            try:
                conn, _ = self._sock.accept()
            except TimeoutError:
                continue
            except OSError:
                return
            t = threading.Thread(
                target=self._handle, args=(conn,), daemon=True
            )
            t.start()

    def _handle(self, conn: socket.socket) -> None:
        try:
            if self._mode == "timeout":
                # Accept and hold; eggdrop's DCC_IDENT timeout
                # (identtimeout seconds) fires and closes from its side.
                conn.settimeout(15.0)
                try:
                    while not self._stop.is_set():
                        chunk = conn.recv(4096)
                        if not chunk:
                            return
                except OSError:
                    return
            else:  # "respond"
                conn.settimeout(5.0)
                data = b""
                while b"\n" not in data and len(data) < 256:
                    chunk = conn.recv(64)
                    if not chunk:
                        break
                    data += chunk
                line = data.decode("ascii", "replace").rstrip("\r\n")
                reply = f"{line} : USERID : UNIX : {self._user}\r\n"
                with contextlib.suppress(OSError):
                    conn.sendall(reply.encode("ascii"))
        finally:
            with contextlib.suppress(OSError):
                conn.shutdown(socket.SHUT_RDWR)
            conn.close()

    def stop(self) -> None:
        self._stop.set()
        if self._sock is not None:
            with contextlib.suppress(OSError):
                self._sock.close()
            self._sock = None
        if self._thread is not None:
            self._thread.join(timeout=2)
            self._thread = None

    def __enter__(self) -> IdentServer:
        return self.start()

    def __exit__(self, *args: object) -> None:
        self.stop()
