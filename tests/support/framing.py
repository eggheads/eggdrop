r"""Line-delimited framing for the test bridge.

Wire format (one line per frame, terminated by `\n`):

    request:   <escaped command>\n
    response:  OK <escaped payload>\n     (success)
    response:  ERR <escaped payload>\n    (Tcl error)

Backslash, newline, and CR are backslash-escaped in the payload so the frame
is always exactly one line. The escape character set is closed: every `\`
in an encoded payload is the start of a `\\`, `\n`, or `\r` sequence.

Telnet-friendly: you can `nc 127.0.0.1 <port>` and type commands by hand.
"""

from __future__ import annotations


class ProtocolError(Exception):
    """Raised on a malformed frame (bad escape sequence or framing)."""


def escape(s: str) -> str:
    """Escape `\\`, `\\n`, `\\r` so the result fits on a single wire line."""
    return (
        s.replace("\\", "\\\\")
        .replace("\n", "\\n")
        .replace("\r", "\\r")
    )


def unescape(s: str) -> str:
    """Inverse of `escape`. Raises `ProtocolError` on dangling/unknown escapes."""
    out: list[str] = []
    i = 0
    n = len(s)
    while i < n:
        c = s[i]
        if c != "\\":
            out.append(c)
            i += 1
            continue
        if i + 1 >= n:
            raise ProtocolError("dangling backslash at end of payload")
        nxt = s[i + 1]
        if nxt == "\\":
            out.append("\\")
        elif nxt == "n":
            out.append("\n")
        elif nxt == "r":
            out.append("\r")
        else:
            raise ProtocolError(f"unknown escape sequence: \\{nxt}")
        i += 2
    return "".join(out)


def encode_request(cmd: str) -> bytes:
    """Frame a Tcl command for transmission to the bridge listener."""
    return (escape(cmd) + "\n").encode("utf-8")


def encode_response(tag: str, payload: str) -> bytes:
    """Frame an `OK`/`ERR` reply (used by the Tcl side; tests use the bridge for input)."""
    return (tag + " " + escape(payload) + "\n").encode("utf-8")


def parse_response(line: str) -> tuple[str, str]:
    """Parse one response line into (tag, payload). Strips trailing newline."""
    line = line.rstrip("\n").rstrip("\r")
    tag, sep, payload = line.partition(" ")
    if not sep and tag in ("OK", "ERR"):
        # tag with empty payload (no separator, no escaped content)
        return tag, ""
    return tag, unescape(payload)
