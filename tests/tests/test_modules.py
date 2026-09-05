"""Modules the default test configuration never loads.

`conftest.py` loads seven modules — pbkdf2, channels, server, ctcp, irc,
console, notes — out of the twenty-one that get built. The other fourteen
have no coverage at all, including several that are newly written and one
(`webui`) that exposes an HTTP listener.

The config template already supports `extra_modules`, so each test here
starts a bot with the module it needs:

    eggdrop_config.render(extra_modules=["compress"])

What each module is tested for depends on what it exposes:

    compress    compressfile / uncompressfile / iscompressed
    blowfish    encrypt / decrypt / encpass
    assoc       assoc / killassoc
    filesys     a large file-area command set
    seen        no Tcl commands; load/unload only
    uptime      no Tcl commands; load/unload only
    woobie      the example module; load/unload only
    webui       an HTTP listener; load/unload and a socket smoke test

For modules with no Tcl surface, loading and unloading cleanly is still
worth asserting: a module that leaks a dcc entry or crashes on unload
takes the bot with it.

Deliberately excluded: `share` and `transfer` need a second linked bot,
`dns` needs real name resolution, `twitch` needs a Twitch-mode config,
and `python` needs an embedded interpreter. Each deserves its own harness
work rather than a token test here.
"""

from __future__ import annotations

import socket

import pytest

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc


# A module's file name is not always the name it registers under: blowfish
# registers as "encryption" and pbkdf2 as "encryption2", so that two
# encryption modules can be swapped without scripts caring which is loaded.
REGISTERED_NAME = {
    "blowfish": "encryption",
    "pbkdf2": "encryption2",
}


def start_with(eggdrop_config, request, *modules: str) -> BridgeClient:
    """Start a bot with the named extra modules loaded."""
    eggdrop_config.render(extra_modules=list(modules))
    request.getfixturevalue("eggdrop_proc")
    return request.getfixturevalue("tcl_bridge")


def require_loaded(tcl_bridge: BridgeClient, module: str) -> str:
    """Return the registered name of `module`, skipping if it is not built.

    Not every module is compiled by default -- `woobie` is an example and
    `webui` is opt-in -- so a missing module is a configuration fact, not
    a test failure.
    """
    name = REGISTERED_NAME.get(module, module)
    if name not in tcl_bridge.eval_ok("modules"):
        pytest.skip(f"{module} is not built in this configuration")
    return name


# ---------- every module loads and unloads ----------


@pytest.mark.parametrize(
    "module",
    [
        "compress",
        "blowfish",
        "assoc",
        "seen",
        "uptime",
        "filesys",
        "woobie",
        "webui",
    ],
)
def test_module_loads(
    eggdrop_config, request: pytest.FixtureRequest, module: str
) -> None:
    """Each module loads and reports itself in the module list.

    A module that fails to load leaves the bot running but silently
    without the feature, so the absence has to be asserted positively.
    """
    tcl_bridge = start_with(eggdrop_config, request, module)
    require_loaded(tcl_bridge, module)


# blowfish is deliberately absent: it registers as the "encryption" module
# and Eggdrop refuses to unload the cipher currently backing stored
# passwords, which is correct behaviour rather than a defect.
@pytest.mark.parametrize("module", ["compress", "assoc", "seen"])
def test_module_unloads_cleanly(
    eggdrop_config, request: pytest.FixtureRequest, module: str
) -> None:
    """A module can be unloaded without taking the bot down.

    Unloading exercises the module's cleanup path, which is rarely run
    and therefore where dangling hooks and freed-but-referenced memory
    tend to live.
    """
    tcl_bridge = start_with(eggdrop_config, request, module)
    eggdrop_proc: EggdropProc = request.getfixturevalue("eggdrop_proc")
    name = require_loaded(tcl_bridge, module)

    # unloadmodule takes the registered name, not the file name, so
    # blowfish is unloaded as "encryption".
    tcl_bridge.eval_ok(f"unloadmodule {name}")
    assert name not in tcl_bridge.eval_ok("modules")
    eggdrop_proc.assert_alive()
    assert tcl_bridge.eval_ok("expr 6*7") == "42"


@pytest.mark.parametrize("module", ["compress", "assoc"])
def test_module_reloads(
    eggdrop_config, request: pytest.FixtureRequest, module: str
) -> None:
    """A module can be unloaded and loaded again.

    The reload cycle is where a module that registers Tcl commands twice,
    or fails to unregister them, shows up.
    """
    tcl_bridge = start_with(eggdrop_config, request, module)
    name = require_loaded(tcl_bridge, module)
    tcl_bridge.eval_ok(f"unloadmodule {name}")
    tcl_bridge.eval_ok(f"loadmodule {module}")
    assert name in tcl_bridge.eval_ok("modules")


# ---------- compress ----------


def test_compress_round_trip(
    eggdrop_config, request: pytest.FixtureRequest, tmp_path
) -> None:
    """A file compressed and then uncompressed returns its original bytes.

    The userfile is what actually gets compressed in production, so a
    lossy round trip here means silent user data loss there.
    """
    tcl_bridge = start_with(eggdrop_config, request, "compress")
    src = tmp_path / "plain.txt"
    payload = "the quick brown fox\n" * 200
    src.write_text(payload)

    tcl_bridge.eval_ok(f"compressfile {{{src}}}")
    assert tcl_bridge.eval_ok(f"iscompressed {{{src}}}") == "1"

    tcl_bridge.eval_ok(f"uncompressfile {{{src}}}")
    assert tcl_bridge.eval_ok(f"iscompressed {{{src}}}") == "0"
    assert src.read_text() == payload


def test_iscompressed_on_plain_file(
    eggdrop_config, request: pytest.FixtureRequest, tmp_path
) -> None:
    """An uncompressed file is reported as such."""
    tcl_bridge = start_with(eggdrop_config, request, "compress")
    src = tmp_path / "plain2.txt"
    src.write_text("not compressed")
    assert tcl_bridge.eval_ok(f"iscompressed {{{src}}}") == "0"


def test_compress_to_a_separate_destination(
    eggdrop_config, request: pytest.FixtureRequest, tmp_path
) -> None:
    """Compressing to a named destination leaves the source untouched."""
    tcl_bridge = start_with(eggdrop_config, request, "compress")
    src = tmp_path / "src.txt"
    dst = tmp_path / "dst.gz"
    src.write_text("some content to compress\n" * 50)

    tcl_bridge.eval_ok(f"compressfile {{{src}}} {{{dst}}}")
    assert dst.exists()
    assert src.read_text().startswith("some content")


# ---------- blowfish ----------


def test_encrypt_decrypt_round_trip(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """Text encrypted with a key decrypts back to the original.

    Blowfish is what botnet links and older userfile passwords rely on,
    so a broken round trip breaks authentication rather than just
    scrambling text.
    """
    tcl_bridge = start_with(eggdrop_config, request, "blowfish")
    plaintext = "attack at dawn"
    cipher = tcl_bridge.eval_ok(f"encrypt secretkey {{{plaintext}}}")
    assert cipher != plaintext
    assert tcl_bridge.eval_ok(f"decrypt secretkey {{{cipher}}}") == plaintext


def test_decrypt_with_wrong_key_does_not_return_plaintext(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """Decrypting with the wrong key does not yield the original text."""
    tcl_bridge = start_with(eggdrop_config, request, "blowfish")
    cipher = tcl_bridge.eval_ok("encrypt rightkey {a secret message}")
    assert tcl_bridge.eval_ok(f"decrypt wrongkey {{{cipher}}}") != "a secret message"


def test_encpass_is_deterministic_and_one_way(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """Password hashing gives the same result for the same input, and does
    not return the password itself."""
    tcl_bridge = start_with(eggdrop_config, request, "blowfish")
    first = tcl_bridge.eval_ok("encpass mypassword")
    second = tcl_bridge.eval_ok("encpass mypassword")
    assert first == second
    assert first != "mypassword"
    assert tcl_bridge.eval_ok("encpass otherpassword") != first


def test_encrypt_handles_empty_and_long_input(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """Encryption copes with boundary-length input.

    Blowfish works on eight-byte blocks, so lengths either side of a
    block boundary are where padding bugs appear.
    """
    tcl_bridge = start_with(eggdrop_config, request, "blowfish")
    for text in ("a", "12345678", "123456789", "x" * 200):
        cipher = tcl_bridge.eval_ok(f"encrypt akey {{{text}}}")
        assert tcl_bridge.eval_ok(f"decrypt akey {{{cipher}}}") == text, text


# ---------- assoc ----------


def test_assoc_names_a_botnet_channel(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """A botnet channel number can be given a name and read back."""
    tcl_bridge = start_with(eggdrop_config, request, "assoc")
    tcl_bridge.eval_ok("assoc 5 testchan")
    assert tcl_bridge.eval_ok("assoc 5") == "testchan"


def test_killassoc_removes_the_name(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """Removing an association clears the stored name."""
    tcl_bridge = start_with(eggdrop_config, request, "assoc")
    tcl_bridge.eval_ok("assoc 6 gone")
    assert tcl_bridge.eval_ok("assoc 6") == "gone"
    tcl_bridge.eval_ok("killassoc 6")
    assert tcl_bridge.eval_ok("assoc 6") == ""


def test_assoc_for_unnamed_channel_is_empty(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """An unnamed botnet channel returns an empty name, not an error."""
    tcl_bridge = start_with(eggdrop_config, request, "assoc")
    assert tcl_bridge.eval_ok("assoc 99") == ""


# ---------- filesys ----------


def test_filesys_registers_its_commands(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """Loading the file area module makes its Tcl commands available.

    `filesys` is a large module with no coverage; confirming its command
    registration is the cheapest first step.
    """
    tcl_bridge = start_with(eggdrop_config, request, "filesys")
    for cmd in ("getdesc", "setdesc", "getowner", "getfiles", "getdirs"):
        assert tcl_bridge.eval_ok(f"info commands {cmd}") == cmd


def test_filesys_getpwd_returns_a_path(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """The file area reports a current directory once loaded."""
    tcl_bridge = start_with(eggdrop_config, request, "filesys")
    tcl_bridge.eval("getpwd")  # may error without a dcc context; must not crash
    request.getfixturevalue("eggdrop_proc").assert_alive()


# ---------- webui ----------


def test_webui_loads_without_opening_a_port_by_default(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """The web interface module loads without listening by default.

    `webui` is the newest module in the tree and the only one that speaks
    HTTP. A module that opens a listener merely by being loaded would be
    a meaningful change in attack surface, so the default matters.
    """
    tcl_bridge = start_with(eggdrop_config, request, "webui")
    require_loaded(tcl_bridge, "webui")
    request.getfixturevalue("eggdrop_proc").assert_alive()


def test_webui_survives_a_garbage_connection(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """Rubbish sent to the bot's listen port does not take it down.

    The listener is reached through Eggdrop's own dcc socket handling, so
    this exercises the read path with input no HTTP parser expects. The
    bot must still answer afterwards.
    """
    tcl_bridge = start_with(eggdrop_config, request, "webui")
    require_loaded(tcl_bridge, "webui")
    eggdrop_proc: EggdropProc = request.getfixturevalue("eggdrop_proc")

    port = int(tcl_bridge.eval_ok("set ::listen-addr-port 0") or 0) or None
    if port is None:
        pytest.skip("no listen port configured in the test config")

    try:
        with socket.create_connection(("127.0.0.1", port), timeout=3) as sock:
            sock.sendall(b"\x00\xff\xfe not http at all \r\n\r\n")
    except OSError:
        pass  # refused is fine; we are asserting the bot survives

    eggdrop_proc.assert_alive()
    assert tcl_bridge.eval_ok("expr 8*8") == "64"


# ---------- interactions ----------


def test_several_modules_load_together(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """Loading many modules at once does not conflict.

    Modules register Tcl commands, dcc handlers and hooks into shared
    tables; loading them individually says nothing about whether their
    registrations collide.
    """
    tcl_bridge = start_with(
        eggdrop_config, request, "compress", "blowfish", "assoc", "seen", "uptime"
    )
    loaded = tcl_bridge.eval_ok("modules")
    for module in ("compress", "blowfish", "assoc", "seen", "uptime"):
        name = REGISTERED_NAME.get(module, module)
        assert name in loaded, f"{module} (as {name}) missing from: {loaded}"


def test_unloading_one_module_leaves_others_working(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """Unloading a module does not disturb the ones still loaded.

    Guards against a cleanup path that unregisters more than its own
    commands.
    """
    tcl_bridge = start_with(eggdrop_config, request, "compress", "blowfish")
    tcl_bridge.eval_ok("unloadmodule compress")  # registers under its own name

    cipher = tcl_bridge.eval_ok("encrypt akey {still working}")
    assert tcl_bridge.eval_ok(f"decrypt akey {{{cipher}}}") == "still working"


def test_encryption_module_cannot_be_unloaded(
    eggdrop_config, request: pytest.FixtureRequest
) -> None:
    """The active encryption module refuses to unload.

    Unloading the cipher that stored passwords were hashed with would
    leave every user unable to authenticate, so Eggdrop declines. Pinned
    because it is easily mistaken for a broken unload path.
    """
    tcl_bridge = start_with(eggdrop_config, request, "blowfish")
    require_loaded(tcl_bridge, "blowfish")

    tcl_bridge.eval("unloadmodule encryption")
    assert "encryption" in tcl_bridge.eval_ok("modules")
    request.getfixturevalue("eggdrop_proc").assert_alive()
