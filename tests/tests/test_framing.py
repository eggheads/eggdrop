"""Unit tests for the line-delimited framing helpers."""

from __future__ import annotations

import pytest

from support.framing import (
    ProtocolError,
    encode_request,
    encode_response,
    escape,
    parse_response,
    unescape,
)


@pytest.mark.parametrize(
    ("raw", "encoded"),
    [
        ("", ""),
        ("hello", "hello"),
        ("foo bar", "foo bar"),
        ("a\\b", "a\\\\b"),
        ("a\nb", "a\\nb"),
        ("a\rb", "a\\rb"),
        ("\\", "\\\\"),
        ("\n", "\\n"),
        ("\r", "\\r"),
        ("\\n", "\\\\n"),  # literal backslash + n, NOT a newline
        ("a\\\nb", "a\\\\\\nb"),  # backslash then newline
        ("héllo", "héllo"),  # unicode passes through
    ],
)
def test_escape_unescape_roundtrip(raw: str, encoded: str) -> None:
    assert escape(raw) == encoded
    assert unescape(encoded) == raw


def test_unescape_dangling_backslash_raises() -> None:
    with pytest.raises(ProtocolError, match="dangling backslash"):
        unescape("foo\\")


def test_unescape_unknown_escape_raises() -> None:
    with pytest.raises(ProtocolError, match="unknown escape"):
        unescape("foo\\x")


def test_encode_request_appends_newline() -> None:
    assert encode_request("set foo 42") == b"set foo 42\n"


def test_encode_request_escapes_newline() -> None:
    assert encode_request("a\nb") == b"a\\nb\n"


def test_encode_response_format() -> None:
    assert encode_response("OK", "42") == b"OK 42\n"
    assert encode_response("ERR", "bad") == b"ERR bad\n"


def test_encode_response_escapes_payload() -> None:
    assert encode_response("OK", "line1\nline2") == b"OK line1\\nline2\n"


def test_parse_response_ok() -> None:
    assert parse_response("OK 42\n") == ("OK", "42")


def test_parse_response_err() -> None:
    assert parse_response("ERR bad command\n") == ("ERR", "bad command")


def test_parse_response_empty_payload() -> None:
    assert parse_response("OK\n") == ("OK", "")
    assert parse_response("OK \n") == ("OK", "")


def test_parse_response_unescapes() -> None:
    assert parse_response("OK line1\\nline2\n") == ("OK", "line1\nline2")
    assert parse_response("OK a\\\\b\n") == ("OK", "a\\b")


def test_parse_response_strips_crlf() -> None:
    assert parse_response("OK 42\r\n") == ("OK", "42")


@pytest.mark.parametrize(
    "payload",
    [
        "",
        "simple",
        "with spaces and stuff",
        "newlines\nare\nfine",
        "carriage\rreturn",
        "back\\slash",
        "all\\of\nthe\rabove",
        "unicode: héllo, 日本語",
        "a" * 4096,
    ],
)
def test_response_roundtrip(payload: str) -> None:
    framed = encode_response("OK", payload)
    assert framed.endswith(b"\n")
    assert framed.count(b"\n") == 1, "frame must be exactly one line"
    line = framed.decode("utf-8")
    assert parse_response(line) == ("OK", payload)
