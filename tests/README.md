# Eggdrop integration tests

Pytest harness for spawning the real Eggdrop binary against a mock IRCd
and asserting on its internal state. Lives outside `src/` so it adds zero
risk to the bot itself: nothing here changes Eggdrop's source.

## Quick start

```sh
make                                 # build the eggdrop binary in repo root
cd tests
uv sync                              # set up the .venv from pyproject.toml
uv run pytest                        # run the suite
uv run pytest -v -k partyline        # run a subset
```

`uv tool run ruff check .` and `uv tool run ty check .` should both report
clean. CI runs them.

## Architectural approach

```
                     ┌──────────────────── per-test tmpdir ────────────────────┐
                     │  eggdrop.conf  eggdrop.user  eggdrop.log  bridge.port    │
                     └─────────────▲───────────────────────────────────▲────────┘
                                   │ rendered                          │ written
                                   │ (jinja2)                          │ at startup
                                   │                                   │
   ┌─────────────┐                 │                       ┌───────────┴──────┐
   │   pytest    │                 │                       │ test_bridge.tcl  │
   │   test fn   │                 │                       │ (sourced from    │
   └──────┬──────┘                 │                       │  eggdrop.conf)   │
          │                        │                       └─────────▲────────┘
          │ uses fixtures          │                                 │
          │                        │                                 │ Tcl
          │                        │                                 │ eval
          │                        │                                 │
          │   ┌─────────────────┐  │   ┌────────────────────────┐    │
          ├──▶│   mock_ircd     │◀─┼──▶│  eggdrop subprocess    │◀───┤
          │   │  (asyncio TCP,  │  │   │   eggdrop -n / -nt     │    │
          │   │   sync facade)  │  │   │   <stdout drained to   │    │
          │   └─────────────────┘  │   │    eggdrop.stdout.log> │    │
          │            ▲           │   └─────────┬──────────────┘    │
          │            │ IRC       │             │ stdin (partyline) │
          │            │ TCP       │             │ if -nt mode       │
          │            │           │             │                   │
          │   ┌────────┴───────┐   │   ┌─────────┴──────────┐  ┌────┴────────┐
          └──▶│  tcl_bridge    │◀──┘   │ send_partyline()   │  │ eval_ok()   │
              │ (TCP client,   │       │ on EggdropProc     │  │ on Bridge-  │
              │  framing.py)   │       │                    │  │ Client      │
              └────────────────┘       └────────────────────┘  └─────────────┘
```

Three loops in play:

1. **Mock IRCd** speaks RFC 1459 + IRCv3 CAP just well enough for Eggdrop to
   register, join, and exchange messages. Async internally; sync API for tests
   (`mock_ircd.recv()`, `.send_welcome()`, `.expect_recv_match()`).
2. **Eggdrop subprocess** runs unmodified. Its `eggdrop.conf` is rendered per
   test from `templates/eggdrop.conf.j2`, points at the mock IRCd's port, and
   sources `support/test_bridge.tcl` when `EGGDROP_TEST=1` is set.
3. **Tcl bridge** is a tiny `socket -server` listener inside Eggdrop that
   takes line-delimited Tcl commands and returns escaped results. The Python
   client (`BridgeClient.eval_ok("...")`) lets a test inspect any internal
   state reachable from the Tcl interpreter — channels, users, settings,
   bind tables, raw variables.

Tests assert on **state** (via the bridge) rather than on text scraped from
logs or partyline output, which is more robust. Logs (`eggdrop.log`,
`eggdrop.stdout.log`) survive in the tmpdir for debugging and are attached
to pytest's failure report.

### Why the bridge instead of `.tcl` over partyline?

The `.tcl` partyline command works but interleaves results with log lines and
mangles multi-line output through `dumplots`. The bridge is a separate
unbuffered channel with explicit framing and no dependence on the partyline
prompt cycle, so introspection is reliable and concurrent with whatever the
partyline is doing.

### Why a real subprocess instead of linking Eggdrop as a library?

To keep changes to Eggdrop at zero. Library-ification of a process built
around `main()` and lots of process-lifetime globals (interp, dcc table,
userlist, channels, modules, signal handlers, OpenSSL, dns child, …) is a
large refactor. Subprocess + bridge gets us regression coverage today and
keeps the door open for a future shared-lib path if it's ever justified.

## Quick start: a test that drives the IRCd and a partyline command

This is `tests/test_partyline_chan.py` (also runnable as
`uv run pytest -k partyline_add`):

```python
import pytest
from support.eggdrop_proc import EggdropProc
from support.bridge_client import BridgeClient
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


def _complete_registration(mock_ircd: MockIrcd) -> None:
    mock_ircd.wait_for_connect(timeout=10.0)
    for _ in range(2):                       # NICK + USER
        mock_ircd.recv(timeout=5.0)
    mock_ircd.send_welcome(nick="TestBot")   # 001-004 + 376
    mock_ircd.drain_until(lambda l: l.startswith("JOIN "), timeout=10.0)
    mock_ircd.send(":mock.test 353 TestBot = #test :@TestBot")
    mock_ircd.send(":mock.test 366 TestBot #test :End of /NAMES")


@pytest.mark.partyline                       # ← spawns eggdrop with -nt
def test_partyline_add_channel(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    _complete_registration(mock_ircd)
    assert tcl_bridge.eval_ok("llength [channels]") == "1"

    eggdrop_proc.send_partyline(".+chan #pytest")

    wait_for(
        lambda: tcl_bridge.eval_ok(
            'expr {[lsearch [channels] "#pytest"] >= 0}'
        ) == "1",
        timeout=5.0,
        description="partyline .+chan #pytest to register",
    )
```

What this exercises:

1. **IRCd dialogue**: TCP connect → CAP/NICK/USER (auto-handled) → welcome
   → JOIN → NAMES. The mock IRCd auto-PONGs and auto-handles `CAP LS` so
   tests don't repeat that boilerplate.
2. **Partyline command**: `eggdrop_proc.send_partyline(".+chan #pytest")`
   writes to Eggdrop's stdin. The `@pytest.mark.partyline` marker tells the
   `eggdrop_proc` fixture to spawn with `-nt` instead of `-n`, opening the
   HQ partyline on stdin. The HQ user (`-HQ`) gets full owner perms
   automatically — no auth handshake.
3. **State assertion via bridge**: `wait_for(...)` polls
   `tcl_bridge.eval_ok(...)` until the Tcl side reports the new channel.
   Polling is needed because `.+chan` runs through Eggdrop's event loop
   asynchronously from the stdin write; `wait_for` has an explicit timeout
   instead of `time.sleep()`.

## Fixtures

| Fixture | Scope | What you get |
| --- | --- | --- |
| `tmp_eggdir` | function | `Path` to per-test scratch dir (alias of `tmp_path`) |
| `mock_ircd` | function | Started `MockIrcd` listening on `127.0.0.1:0` |
| `eggdrop_config` | function | `EggdropConfig` with `.render(**overrides)` to customise the conf |
| `eggdrop_proc` | function | Spawned `EggdropProc`. `-nt` if test has `@pytest.mark.partyline` |
| `tcl_bridge` | function | Connected `BridgeClient` ready for `.eval_ok("...")` |
| `_process_tracker` | session, autouse | Backstop kill of any leaked eggdrop pids |

The fixtures wire to each other: pulling in `tcl_bridge` is enough — it
depends on `eggdrop_proc` which depends on `eggdrop_config` and
`mock_ircd`, all of which depend on `tmp_eggdir`.

## Customising a test's eggdrop.conf

Render with overrides before the proc starts:

```python
def test_with_custom_nick(eggdrop_config, eggdrop_proc, mock_ircd, tcl_bridge):
    eggdrop_config.render(
        nick="OtherBot",
        channels=[
            {"name": "#a", "chanmode": "+nt"},
            {"name": "#b", "chanmode": "+ntk", "key": "secret"},
        ],
        extra_tcl="set my-test-var 42\n",
    )
    # ...rest of the test
```

If you don't call `render()`, the `eggdrop_proc` fixture renders with
defaults from `EggdropConfig.context()`.

## Layout

```
tests/
├── pyproject.toml               # uv project, pytest+ruff+ty config
├── README.md                    # this file
├── conftest.py                  # all fixtures + failure-report hook
├── support/
│   ├── framing.py               # line-delimited \-escaped frame format
│   ├── test_bridge.tcl          # sourced inside Eggdrop, opens TCP listener
│   ├── bridge_client.py         # Python client → eval_ok("...")
│   ├── mock_ircd.py             # asyncio IRCd, sync facade
│   ├── eggdrop_proc.py          # subprocess wrapper, stdout drain, terminate
│   └── waiters.py               # wait_for / wait_for_file / wait_for_log_match
├── templates/
│   ├── eggdrop.conf.j2
│   └── userfile.j2
└── tests/
    ├── test_framing.py
    ├── test_smoke_connect.py
    └── test_partyline_chan.py
```

## Bridge wire protocol

Telnet-friendly, one frame per line, `\` `\n` `\r` backslash-escaped:

```
$ nc 127.0.0.1 <port>
set ::nick
OK TestBot
expr 2 + 2
OK 4
nosuch
ERR invalid command name "nosuch"
```

Request: `<escaped command>\n`. Response: `OK <escaped result>\n` or
`ERR <escaped result>\n`. The bridge only listens when the spawn env has
`EGGDROP_TEST=1`, so production Eggdrops sourcing the same config skip it.

## Environment knobs

- `EGGDROP_BIN` — absolute path to the eggdrop binary. Default: `<repo>/eggdrop`.
- `EGGDROP_SRC` — absolute path to the eggdrop source tree (for `mod-path`,
  `help-path`, `EGG_LANGDIR`). Default: parent of `tests/`.

## Debugging a failing test

- Each test's runtime files survive at `/tmp/pytest-of-<user>/pytest-current/<nodeid>/`.
  pytest preserves the last 3 sessions automatically.
- Two logs are written:
  - `eggdrop.log` — Eggdrop's own log (raw IRC `[@]` incoming, `[m->]`/`[s->]`
    outgoing, plus messages, channels, output, file ops).
  - `eggdrop.stdout.log` — everything Eggdrop wrote to stdout/stderr.
- On failure, both are attached to the pytest report.
- For verbose live output: `uv run pytest -s --log-cli-level=DEBUG path::to::test`.
- To poke the bridge by hand from a hung test, copy the port out of
  `<tmp>/bridge.port` and `nc 127.0.0.1 <port>`.

## Why some things are the way they are

- **`mod-path` set before `loadmodule`.** Eggdrop reads `mod-path` at each
  `loadmodule` call, not lazily, so the template orders them accordingly.
- **`loadmodule pbkdf2` first.** The userfile read enforces that the
  encryption module is present (src/main.c:1076).
- **`EGG_LANGDIR` env var.** Avoids a symlink in every tmpdir; the language
  files load from the source tree directly.
- **`set msg-rate 0`.** Combined with eggdrop's one-msg-per-second
  `HOOK_SECONDLY` dequeue, this keeps tests fast without altering
  protocol semantics. WHOIS still goes out before JOIN; tests use
  `mock_ircd.drain_until(lambda l: l.startswith("JOIN "))` rather than
  `expect_recv_match` to skip past it.
- **Bridge socket vs `.tcl` over partyline.** The bridge has clean framing
  and no log interleaving; the partyline is for tests of the partyline UX.
- **No `time.sleep` in tests.** Use `waiters.wait_for(...)` /
  `wait_for_file(...)` / `mock_ircd.drain_until(...)` — every wait has an
  explicit timeout and a description.

## What's intentionally out of scope (for now)

- Linking eggdrop as a shared library / refactoring globals out.
- C-side test hooks; everything stays in Tcl/Python.
- SSL/TLS to the mock IRCd. Plain TCP only.
- Botnet (`share`, `transfer` modules), DCC chat over TCP, Python module.
- Real passwords. The owner in the userfile uses `pass = "-"` (no auth);
  the bridge bypasses the need for it. Partyline auth tests are deferred
  until there's a reason for them.
