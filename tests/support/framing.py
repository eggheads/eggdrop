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
    pass


def escape(s: str) -> str:
    return (
        s.replace("\\", "\\\\")
        .replace("\n", "\\n")
        .replace("\r", "\\r")
    )


def unescape(s: str) -> str:
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
    return (escape(cmd) + "\n").encode("utf-8")


def encode_response(tag: str, payload: str) -> bytes:
    return (tag + " " + escape(payload) + "\n").encode("utf-8")


def parse_response(line: str) -> tuple[str, str]:
    """Parse one response line into (tag, payload). Strips trailing newline."""
    line = line.rstrip("\n").rstrip("\r")
    tag, sep, payload = line.partition(" ")
    if not sep and tag in ("OK", "ERR"):
        # tag with empty payload (no separator, no escaped content)
        return tag, ""
    return tag, unescape(payload)
