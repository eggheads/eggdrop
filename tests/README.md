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

from support.bridge_client import BridgeClient
from support.eggdrop_proc import EggdropProc
from support.irc_helpers import drive_join_with_names, drive_registration
from support.mock_ircd import MockIrcd
from support.waiters import wait_for


@pytest.mark.partyline                       # ← spawns eggdrop with -nt
def test_partyline_add_channel(
    eggdrop_proc: EggdropProc,
    mock_ircd: MockIrcd,
    tcl_bridge: BridgeClient,
) -> None:
    drive_registration(mock_ircd)            # NICK + USER → welcome
    drive_join_with_names(mock_ircd, "@TestBot")  # JOIN echo + NAMES + WHO + ...

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
   → JOIN → NAMES → bot's post-join MODE/WHO queries serviced. The mock
   IRCd auto-PONGs and auto-handles `CAP LS`; the helpers drive the rest.
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

## Helpers (`support/irc_helpers.py`)

Shared multi-step IRC interactions so tests don't repeat boilerplate.

### `drive_registration(mock_ircd, nick="TestBot", isupport_tokens=None)`

Drives Eggdrop through IRC registration:

1. Waits for the bot's TCP connect.
2. Drains the bot's `NICK` and `USER` (in either order).
3. Sends the welcome sequence (001-004, optional 005 with
   `isupport_tokens`, 376 end-of-MOTD).

`isupport_tokens` is a list of raw `KEY=VALUE` (or bare `KEY`) strings
that go into a single 005 line. Use this to test parsing of specific
tokens, e.g.:

```python
drive_registration(mock_ircd, isupport_tokens=[
    "PREFIX=(qaohv)~&@%+",
    "CHANMODES=beI,kLf,l,psmntirzMQNRTOVKDdGPZSCc",
])
```

After this returns, Eggdrop has processed 005 and is about to JOIN
configured channels.

### `drive_join_with_names(mock_ircd, members_with_prefix, nick="TestBot", server="mock.test") -> str`

Mimics a real IRCd's full post-JOIN dance for the bot:

1. Waits for the bot's `JOIN #chan`.
2. Echoes `:nick!u@h JOIN :#chan` back so Eggdrop populates `chan->name`
   and considers itself joined.
3. Sends `353` NAMES with `members_with_prefix` (a NAMES-style string
   like `"@TestBot ~bigboss +regular"`) and `366` end-of-NAMES.
4. Drains the post-join queries Eggdrop fires off:
   - `MODE +b/+e/+I` → empty `368/349/347` end-of-list replies
   - `WHO #chan` → one `352` per member (prefix symbols passed through to
     the WHO flags field, so `opchars`-based op detection picks them up)
     followed by `315` end-of-WHO
5. Leaves `MODE #chan` (no list flag) **unanswered** so individual tests
   can send their own `324` mode reply if they need to.

Returns the channel name. Quiesces when no new lines arrive for ~300 ms
(or after a 5 s hard cap).

```python
chan = drive_join_with_names(mock_ircd, "@TestBot alice +bob")
# bot is now fully joined to chan; alice is a plain member, bob is voiced
mock_ircd.send(f":mock.test 324 TestBot {chan} +ntk secret")  # custom 324
```

### `wait_for_isupport(bridge, key, expected, timeout=5.0)`

Polls `isupport get <key>` over the bridge until it returns `expected`.
Useful right after `drive_registration(..., isupport_tokens=...)` to
ensure Eggdrop has finished processing 005 before assertions run.

```python
wait_for_isupport(tcl_bridge, "PREFIX", "(qaohv)~&@%+")
```

### `split_member_prefix(token) -> (nick, prefix_symbols)`

Tiny helper used internally by `drive_join_with_names`; exposed for
test code that needs to do the same parsing. `"@alice"` → `("alice", "@")`,
`"~&boss"` → `("boss", "~&")`, `"plain"` → `("plain", "")`.

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
│   ├── irc_helpers.py           # drive_registration, drive_join_with_names, ...
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
