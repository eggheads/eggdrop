"""TCP client for the test_bridge.tcl listener inside Eggdrop."""

from __future__ import annotations

import contextlib
import socket
import time

from .framing import encode_request, parse_response


class BridgeError(Exception):
    """Tcl evaluation returned an ERR frame."""


class BridgeTimeout(Exception):
    """Read from the bridge timed out before a frame arrived."""


class BridgeClient:
    """Synchronous client. One connection, one outstanding request at a time.

    Eggdrop's bridge serves frames in order, so this design is sufficient for
    test code that runs sequentially.
    """

    def __init__(self, host: str, port: int, timeout: float = 5.0) -> None:
        """Open a TCP connection to the bridge. Raises `OSError` if it can't connect."""
        self._host = host
        self._port = port
        self._sock = socket.create_connection((host, port), timeout=timeout)
        self._sock.settimeout(timeout)
        self._buf = bytearray()

    @classmethod
    def connect_with_retry(
        cls, host: str, port: int, total_timeout: float = 5.0
    ) -> BridgeClient:
        """Retry the connect every 50 ms until `total_timeout` elapses.

        Used by the `tcl_bridge` fixture to wait through Eggdrop's startup
        between writing the port file and accepting on it.
        """
        deadline = time.monotonic() + total_timeout
        last: Exception | None = None
        while time.monotonic() < deadline:
            try:
                return cls(host, port)
            except OSError as e:
                last = e
                time.sleep(0.05)
        raise BridgeTimeout(
            f"could not connect to bridge at {host}:{port} "
            f"within {total_timeout}s: {last}"
        )

    def close(self) -> None:
        """Close the TCP connection. Idempotent; safe to call on a closed socket."""
        with contextlib.suppress(OSError):
            self._sock.close()

    def __enter__(self) -> BridgeClient:
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def eval(self, cmd: str, timeout: float = 5.0) -> tuple[bool, str]:
        """Evaluate a Tcl command. Returns (ok, result_text)."""
        self._sock.sendall(encode_request(cmd))
        deadline = time.monotonic() + timeout
        while b"\n" not in self._buf:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise BridgeTimeout(f"no response to {cmd!r} within {timeout}s")
            self._sock.settimeout(remaining)
            try:
                chunk = self._sock.recv(4096)
            except TimeoutError as e:
                raise BridgeTimeout(
                    f"no response to {cmd!r} within {timeout}s"
                ) from e
            if not chunk:
                raise BridgeError(
                    f"bridge closed connection while waiting for "
                    f"response to {cmd!r}"
                )
            self._buf.extend(chunk)
        nl = self._buf.index(b"\n")
        line = bytes(self._buf[:nl]).decode("utf-8", "replace")
        del self._buf[: nl + 1]
        tag, payload = parse_response(line)
        return tag == "OK", payload

    def eval_ok(self, cmd: str, timeout: float = 5.0) -> str:
        """Evaluate a Tcl command and return its result, raising on ERR."""
        ok, result = self.eval(cmd, timeout=timeout)
        if not ok:
            raise BridgeError(f"Tcl error evaluating {cmd!r}: {result}")
        return result
