# arbmodes3 — implementation checklist

Step-by-step checklist for implementing the ISUPPORT-driven arbitrary
channel mode refactor. **Read `ARCHITECTURE.md` first** — it is the
authoritative design record (decisions `D-PFX*`/`D-CHM*`/`D-ISU*`/`D-Q*`/
`D-LST*`/`D-TCL*`, the test plan with test IDs `A1–A31`/`B*`, and the
coverage matrix). This file only sequences the work; when in doubt about
*what* the behaviour should be, ARCHITECTURE.md wins.

## How to use this file

- Work top to bottom. **Do not start a step until the previous step's
  "Gate" checklist is fully checked.** Check boxes as you go (edit this
  file), commit it together with the step's code.
- One commit (or small series) per step, on branch `arbmodes3`. Suggested
  subject prefix: `arbmodes3 step N: …`. Single PR at the end.
- Line numbers below are hints from the planning audit and **will drift**
  — locate code by function name.

## Ground rules (from ARCHITECTURE.md — violations are bugs)

- [x] Read ARCHITECTURE.md in full before writing any code.
- **Source compatibility is paramount.** Legacy fields
  (`chan->channel.mode/.key/.maxmembers`, `memberlist.flags`,
  `mode_pls_prot`/`mode_mns_prot`/`limit_prot`/`key_prot`,
  `pls/mns/cmode/key/rmkey/limit`) stay in the structs and stay *correct*
  (maintained as mirrors). New struct fields are **appended** at the end,
  commented `/* arbmodes: may change — use accessors */`.
- **Readability over LOC.** Merge only true structural twins (op+halfop).
  Keep voice and the six b/e/I list handlers separate.
- **Characterization tests (A1–A31) must stay green at every step.** A red
  A-test is a regression to investigate, not a test to edit — the only
  sanctioned edits are listed in steps 7 and 2 (ARCHITECTURE.md sections
  C/D).
- New code reads mode info via `modecharinfo` accessors
  (`MODE_TYPE`/`MODE_PREFIX`/`MODE_RANK`/…), never hardcoded letters,
  except inside the explicitly letter-specific policy layer.
- Tests: no `time.sleep`; use `wait_for(...)`/`drain_until(...)` with
  descriptions. State assertions via `tcl_bridge`, wire assertions via
  `mock_ircd.expect_recv_match`.

## Build & test commands

```sh
make -j 9                     # incremental build (configure once)
cd tests && uv sync           # one-time venv setup
cd tests && uv run pytest                 # full suite (very slow)
cd tests && uv run pytest -k modes -x     # the mode suites (slow)
cd tests && uv tool run ruff check . && uv tool run ty check .   # lint/type, must be clean
```

---

## Step 0 — Characterization tests + MODES clamp

Freeze current behaviour before touching anything. The only C change in
this step is the out-of-bounds clamp.

### 0.1 C fix: MODES clamp

- [x] In `irc_isupport()` (src/mod/irc.mod/chan.c, `MODES` branch): clamp
      the parsed value to `MODES_PER_LINE_MAX` (src/chan.h) instead of 64.
      `modesperline` must never exceed the `cmode[]` array bound.
- [x] **Bonus bugfix found by characterization (A13):** `got_op()` and
      `got_halfop()` passed an *uninitialized* `s[UHOSTLEN]` to
      `modebind_refresh()` as the victim userhost, so the victim's flag
      record was re-resolved from stack garbage after the bind — `+bitch`
      then deopped even +o users. Fixed by filling `s` like
      `got_deop`/`got_dehalfop`/the `v` case do
      (`simple_sprintf(s, "%s!%s", m->nick, m->userhost)`). Separate
      commit; report upstream-worthy.

### 0.2 Test infrastructure

- [x] Add helper(s) in `tests/support/` as needed: create a test user with
      flags via bridge (`adduser`/`chattr`), install a `bind mode`
      accumulator proc, snapshot/diff outbound IRC lines. Keep them in
      existing modules where they fit (`irc_helpers.py`).
      Added: `wait_onchan`, `create_test_user`, `install_mode_log`/
      `get_mode_log`, and `drive_join_with_names(chanmodes_324=...,
      join_line=...)` (answers the bare join-time `MODE #chan` query —
      without it the op bot queues its desired chanmode at end-of-WHO and
      pollutes wire assertions; see helper docstring).

### 0.3 Characterization suites (test IDs per ARCHITECTURE.md §Test plan A)

Create the files and implement each listed test. **If a test reveals
actual current behaviour differs from the spec sketch, pin the *actual*
behaviour and note the deviation in the test docstring** — step 0 never
changes C behaviour (except 0.1).

- [x] `tests/tests/test_modes_characterization.py`: A1 (getchanmode
      format, flag letters as *set membership*), A2 (bind mode args +
      `wasop` inside bind, `-l`→`""` quirk), A3 (WHO status both WHOX 354
      and plain 352; away `G`→`isaway`; 005 `BOT=B` flag→`isircbot`), A5
      (netsplit WASOP + stopnethack-mode 2 server-reop; FAKEOP deop for
      non-wasop member), A27 (Undernet `324 +k *` → MODE re-ask once
      opped), A29–A31 already exist in `test_isupport_modes.py` —
      verified passing, not moved. A4 lives in
      `test_modes_userhost_in_names.py` (needs a module-local mock_ircd
      override: CAP ACK only enables caps with a record from CAP LS).
- [x] `tests/tests/test_modes_pushmode.py`: A6 (classic pushmode + SENT
      dedup), A7 (pushmode `+k/+l/-k` wire output), A8 (`prevent_mixing`
      e/I two-line split), A9 (`MODES=4` line splitting, args aligned).
- [x] `tests/tests/test_modes_policy.py`: A13 (+bitch; also pins the
      chanset-recheck sweep of existing unauthorized ops), A14
      (protectops/protectfriends re-op), A15 (revenge — requires +revenge
      AND a matching protect* setting per want_to_revenge; +revenge alone
      never punishes), A16 (bot-deop → `bind need` op + SENT clearing;
      mind Tcl brace-quoting of leading-# list elements in accumulators),
      A17 (autoop/autovoice with `aop-delay 0:0`), A22 (desync kick;
      reversal asserted on a prefix mode — flag modes are NOT bounced
      unless also chanmode-protected), A23 (bounce-modes via server `+k`;
      plain flag modes currently only bounce when chanmode-protected,
      negative deliberately unpinned per D10).
- [x] `tests/tests/test_modes_enforcement.py`: A10 (chanmode flag enforce
      live (`+nt-i`) + on-join recheck), A11 (key enforce + JOIN key +
      `-k` re-add + stranger `+k` replaced + `chanmode -k` bounce), A12
      (limit enforce), A18 (enforcebans kick), A19 (ban-on-bot bounce),
      A20 (sticky ban re-add + `-dynamicbans` re-add after unban), A21
      (`-userexempts` bounce), A24 (b/e/I list tracking via numerics +
      live MODE), A25 (chanmode round-trip: `channel set`, partyline
      `.chanset`, savechannels→chanfile→rehash survival; note rehash
      WRITES the chanfile from memory before re-reading, so
      restore-after-memory-change is untestable by design), A26
      (partyline `.op`, `.kickban`), A28 (bind-proc removes channel
      mid-burst → no crash).
- [x] B0 clamp test: 005 `MODES=20`, queue ≥7 modes, `assert_alive()`,
      every emitted `MODE` line has ≤ `MODES_PER_LINE_MAX` mode letters.

### Gate 0

- [x] Build clean (no new warnings in touched files).
- [x] Full `uv run pytest` green (197 tests), including every A-test and B0.
- [x] `ruff check` and `ty check` clean.
- [x] B0-fails-when-reverted: **inapplicable as specified** — verified
      empirically that the per-second `flush_modes()` (irc.c) already
      re-clamps `modesperline` each tick, so the unclamped parse is only
      exposed in a sub-second window that can't be hit deterministically
      from outside. B0 stays as the user-visible contract test (lines
      never exceed `MODES_PER_LINE_MAX`); the parse-time clamp closes the
      remaining UB window. ARCHITECTURE.md updated.
- [x] Committed (clamp, got_op/got_halfop bugfix, and tests as separate
      commits).

---

## Step 1 — `rank` field + mode-index helpers + isupport replay

No behaviour change. Pure infrastructure. Two commits: (a) server.mod
replay, (b) irc.mod rank/helpers.

- [x] server.mod (D-ISU6): parse the default ISUPPORT string eagerly in
      `isupport_init()` (src/mod/server.mod/isupport.c) so records exist
      from module load; keep `isupport_preconnect()` re-applying the
      `isupport-default` Tcl var before each connect (fires binds only on
      change, as today).
- [x] server.mod: add `isupport_replay()` — walk `isupport_list`, re-fire
      the H_isupport bind table for every record with an effective value
      (server value, else default). Export via appended `server_funcs[54]`
      slot + server.h macro.
- [x] irc.mod: call `isupport_replay()` in `irc_start` immediately after
      `add_builtins(H_isupport, irc_isupport_binds)` — covers fresh start
      *and* reload-while-connected for ALL isupport-derived state
      (`use_354`, `modesperline`, `max_*`, `botflag005`, `modecharinfo`).
- [x] Document in doc/sphinx (isupport bind docs) + UPGRADING: isupport
      binds must be idempotent; they may be re-fired with unchanged values
      when a module loads.
- [x] Add `uint8_t rank` to `mode_info_t` (src/mod/irc.mod/irc.h);
      `PREFIX_RANK_NONE` (0xFF) for non-prefix modes.
- [x] Populate `rank` in `process_prefix()` (src/mod/irc.mod/chan.c) from
      position in the PREFIX token (0 = highest). Prefix modes beyond
      `MAX_PREFIX_MODES` (8) keep their full `modecharinfo` entry (type,
      prefix char, rank — parsing/bind/rank-compare stay correct); only
      per-member bit tracking is unavailable; log once at parse time
      (D-PFX9). Also set the sentinel on non-prefix modes in
      `process_chanmodes()`.
- [x] Add helpers: `MODE_RANK(c)` macro + `static inline mode_to_index(c)`
      in irc.h (`a-z`→0–25, `A-Z`→26–51, `0-9`→52–61, else -1);
      `static inline mode_by_prefixchar(c)` in chan.c (scans
      `modecharinfo`). `static inline` keeps them warning-free until their
      first caller lands in steps 2/4.

### Gate 1

- [x] Build clean (no warnings in touched files); full pytest green (all
      A-tests unchanged, 200 total).
- [x] B1 test (`test_modes_isupport_seed.py`): `.status all` shows the
      default PREFIX/CHANMODES parsed *before any connect*; a server's 005
      `Z` flag survives an irc.mod unload/reload while connected. Both
      assertions verified to FAIL with the `isupport_replay()` call removed
      (meaningful regression coverage), pass with it restored.
- [x] Committed (server.mod and irc.mod parts separately).

---

## Step 2 — Member prefix storage + WHO generalization + multi-prefix

Implements D-PFX2/D-PFX5/D-PFX6. `isop`/`me_op` become literal-`o`;
`opchars` is deprecated.

- [x] Repack `mode_info_t` (src/mod/irc.mod/irc.h) into a no-padding,
      byte-comparable layout: `uint8_t type; uint8_t rank; char prefix`.
      Keep `mode_type_t` as the symbolic enum, but store it as `uint8_t`.
      Explicitly write every field in temporary `mode_info_t` arrays
      (`MODETYPE_INVALID`, `PREFIX_RANK_NONE`, `'\0'` for empty entries) so
      PREFIX-change detection can use `memcmp` safely.
- [x] Append to `memberlist` (src/chan.h): `uint8_t prefixmodes,
      wasprefix, sentplus, sentminus` (bit `1<<rank`).
- [x] Member resync on effective PREFIX change (D-PFX8): when
      `update_chanmodes(is_prefix=1)` changes the effective prefix set/order
      (compare old/new prefix entries; do nothing on identical replay), clear
      all members' `prefixmodes`/`wasprefix`/`sentplus`/`sentminus` +
      legacy mirror bits (`CHANOP`/`CHANHALFOP`/`CHANVOICE`,
      `WASOP`/`WASHALFOP`, prefix `SENT*`, and `FAKEOP`/`FAKEHALFOP`) and
      `reset_chan_info(chan, CHAN_RESETWHO)` each active channel.
- [x] Add accessors (irc.mod): `member_has_prefixmode`,
      `member_had_prefixmode`, `member_has_prefixmode_atleast`,
      plus sent-state readers/setters. Keep them private/static in irc.mod
      unless a later step strictly needs C module API exposure. Accessor
      parameters are classified by character class: `a-zA-Z0-9` = mode
      letter, anything else = prefix char via `mode_by_prefixchar()`.
      Setters no-op silently for ranks `>= MAX_PREFIX_MODES`. Use distinct
      setters for current state, `wasprefix`, `sentplus`, and `sentminus`;
      each maintains the legacy mirror: `CHANOP` ⇔ literal `o`,
      `CHANHALFOP` ⇔ `h`, `CHANVOICE` ⇔ `v`, plus `WASOP`/`WASHALFOP` and
      prefix `SENT*` mirroring for o/h/v.
- [x] Zero-initialize newly allocated `memberlist` nodes/sentinels
      (`newmember()` and the `killmember()` fallback sentinel) so appended
      prefix fields never inherit allocator garbage. On netsplit rejoin,
      copy pre-split `prefixmodes` into `wasprefix`, clear current/sent
      prefix state, and preserve the legacy WAS mirrors.
- [x] Rewrite `got352or4()` flags parsing: iterate the WHO flags field and
      set the full prefix bitset via `modecharinfo` (replacing `opchars`,
      hardcoded `'%'`/`'+'`). Keep `'G'` (away), `botflag005`, `H`,
      `STOPWHO` logic as-is. Empty flags (`""` from the NAMES path) clears
      the bitset — preserving A4.
- [x] Convert `gotmode` o/h/v cases, `got_op/got_halfop/got_deop/
      got_dehalfop`, `real_add_mode()` SENT/dup logic, and
      `me_op`/`me_halfop`/`me_voice` to the accessors (state reads/writes
      only — policy untouched in this step). The exported compatibility
      helpers must remain byte-for-byte behaviourally identical; A1 now
      compares `botisop`/`botishalfop`/`botisvoice` with the corresponding
      member-level Tcl commands for the bot.
- [x] Add a narrow generic PREFIX branch in the existing `gotmode()`:
      advertised prefix modes other than o/h/v consume the nick argument,
      update member prefix state, fire `bind mode`, and run
      `modebind_refresh()`, but execute no built-in policy. Preserve
      WAS/wasprefix bind-time semantics: current state changes before the
      bind, `wasprefix` changes after the bind.
- [x] Remove `opchars` recognition; keep the Tcl variable accepted but
      ignored, log a deprecation warning once per irc.mod load when Tcl
      writes it (not for internal default initialization).
- [x] Negotiate `multi-prefix` (D-PFX6): add to server.mod's CAP LS
      request list (servmsg.c:1565 pattern), gated on a server.mod-local
      Tcl config var `multi-prefix` **defaulting to 1** (precedent:
      `extended-join`, servmsg.c:44). Do not export it through
      `server_funcs`. 352/353/354 parsers strip all leading current PREFIX
      chars, plus legacy fallback `@`/`%`/`+` when needed. UPGRADING note
      for raw-bind scripts + the var as escape hatch.
- [x] **Rewrite** `test_names_with_extended_prefix_grants_op_when_opchars_includes_it`
      (tests/tests/test_isupport_modes.py) per ARCHITECTURE.md §D: `~`-only
      owner is *not* `isop`; `~@` owner is `isop` via literal `o`.
      Generic owner state is asserted via step-10 commands once they exist;
      until then, assert literal-o `isop` behaviour, WHO prefix consumption,
      and `bind mode` firing for generic prefixes.
- [x] New tests (B2): multi-prefix REQ'd (`cap enabled`); WHOX 354 with
      `~&@%+` → literal-o `isop` semantics (`~`-only false, `~@` true);
      `opchars` deprecation warning + no effect on `~` recognition; MODE
      `+q nick` (prefix-q net) consumes the nick and fires `bind mode +q
      nick`. Direct `isprefix`/`wasprefix` assertions are deferred to step
      10 with the Tcl commands; arbitrary parameterized outbound `pushmode
      +q nick` waits for the step-6 queue rewrite.

### Gate 2

- [x] Build clean; full pytest green — **all A-tests pass unmodified**
      except the one sanctioned rewrite above.
- [x] B2 tests green.
- [x] Grep gate: no remaining reads of `opchars` in C code
      (`grep -rn opchars src/` → only the deprecated Tcl variable shim).
- [x] Committed.

---

## Step 3 — `can_set_mode()` capability gate

Implements D-PFX4. Replaces `HALFOP_CANTDOMODE`/`HALFOP_CANDOMODE`/
`NOHALFOPS_MODES`.

- [x] Implement `can_set_mode(struct chanset_t *chan, char mode)` in
      irc.mod per D-PFX4: best-rank from the bot's own prefix bitset;
      rank 0 → anything; prefix target → strictly-lower rank only, except
      `o` may set/unset `o`; non-prefix target → `me_op || me_halfop`.
      Honor `NO_HALFOP_CHANMODES` (keep the ifdef).
- [x] Convert every `HALFOP_CANTDOMODE`/`HALFOP_CANDOMODE` call site
      (mode.c, chan.c, cmdsirc.c, msgcmds.c, irc.c — `grep -rn
      HALFOP_CA src/`) to `can_set_mode()`. Delete the macros and
      `NOHALFOPS_MODES` from src/chan.h.
- [x] New tests (B3): bot as `%` → `pushmode +v` emits, `+o`/`+h`
      suppressed, `+b` and a flag mode emit; bot as `@` → `+o`/`-o` emit
      (self-rank exception); quiet-LIST net, bot as `%` → `pushmode +q
      mask` is not capability-suppressed (pre-step-6 legacy queueing may
      emit only the mode letter; arg-carrying generic LIST queueing waits
      for step 6). The old `NOHALFOPS_MODES q` block is gone — this is an
      intentional behaviour fix, note it for the PR.

### Gate 3

- [x] Build clean; full pytest green (A-suite untouched; A26 partyline
      command tests confirm cmdsirc.c conversion).
- [x] B3 green. Grep gate: `grep -rn "NOHALFOPS\|HALFOP_CA" src/` empty.
- [x] Committed.

---

## Step 4 — Channel mode storage + accessors

Implements D-CHM1/D-CHM2. Legacy `channel.mode`/`key`/`maxmembers` become
mirrors.

- [x] Append to `struct chan_t` (src/chan.h): `uint64_t modeflags;
      char *modeargs[62];` with the "may change — use accessors" comment.
- [x] Implement accessors in irc.mod: `chanmode_set(chan, mode, arg)`,
      `chanmode_unset`, `chanmode_isset`, `chanmode_getarg`,
      `chanmode_clear` (frees args; call from reset/part/rejoin paths
      where `channel.mode` is currently zeroed, e.g. `reset_chan_info`,
      got324 entry, channel teardown — find with `grep -n "channel.mode
      = 0" src/mod/irc.mod/`).
- [x] Type-gated legacy mirror inside set/unset (D-CHM2): update the
      legacy `CHAN*` bit only when `MODE_TYPE(c)==MODETYPE_FLAG` *and*
      the letter is one of the historic 20; `k` (KEY-typed) also maintains
      `CHANKEY` + `set_key()`; `l` (LIMIT-typed) also maintains
      `channel.maxmembers`.
- [x] Convert all writers of `channel.mode`/`channel.key`/
      `channel.maxmembers` in irc.mod (`got324`, `gotmode`, `set_key`
      callers, join/reset paths) to the accessors. `getchanmode()` keeps
      rendering from legacy fields in this step.
- [x] `channels.mod` cleanup/accounting detail: `clear_channel`/`init_channel`
      free `modeargs` directly, and `channels_expmem()` counts them because
      shared channel allocations are owned by channels.mod.
- [x] Explicit modes-known state (D-CHM7): flag in the new store, false at
      join/`clear_channel`/reset, set by `got324`;
      `recheck_channel_modes()` additionally gates on it (the
      `CHAN_ASKEDMODES` re-ask semantics stay untouched; `got324` keeps
      running recheck while CHAN_PEND). Kills the enforce-against-zero
      window that forced the `chanmodes_324` test workaround.
- [x] New tests (B4): inbound `MODE +S` (advertised flag) and `324 … j 3:5`
      → state queryable (until step 10 commands exist, assert indirectly:
      legacy fields unchanged for classic modes, and add a temporary
      C-debug or use `getchanmode` for classic; the full assertions land
      with B10 — keep B4 minimal here and extend in step 10). D-CHM7
      test: withhold 324 until after 315 → no chanmode push before 324,
      correct push right after 324 arrives.

### Gate 4

- [x] Build clean; full pytest green (A1/A10–A12/A24/A27 are the canaries
      for mirror correctness).
- [x] Committed.

---

## Step 5 — Generic `gotmode` dispatch + policy merge + generic lists

The big one. Implements D-PFX1/D-PFX7, D-LST1/D-LST2, D10, the
state-vs-bind ordering table, and `bind mode` for all modes.

- [x] Rewrite `gotmode()` (src/mod/irc.mod/mode.c) as `modecharinfo` type
      dispatch per the ordering table in ARCHITECTURE.md §Target design.
      Preserve the preamble exactly (desync/fake-mode kick, `reversing`
      reset, CHAN_ASKEDMODES tail).
- [x] `check_tcl_mode` fires for **every** mode change with type-based
      args (prefix→nick, list→mask, key/limit→arg, flag→`""`; keep the
      `-l`→`""` quirk). Preserve `modebind_refresh` re-entrancy handling
      after every bind call.
- [x] Merge `got_op`+`got_halfop` → parameterized `got_prefixmode`, and
      `got_deop`+`got_dehalfop` → `got_deprefixmode` (CHANOP↔CHANHALFOP,
      protectops↔protecthalfops, autoop↔autohalfop, shared stopnethack;
      op-only extras — revenge, flood-deop, need-op, `deopd` — stay
      op-conditional *visibly*, not buried). Keep the voice policy
      separate. Higher prefixes: state + bind only, **no policy**
      (D-PFX1).
- [x] b/e/I: route through the dispatch, keep
      `got_ban/got_unban/got_exempt/got_unexempt/got_invite/got_uninvite`
      as-is (D-LST1).
- [x] Generic list store for non-b/e/I LIST modes (D-LST2): per-channel
      map mode char → list of `{mask, who, time}`; update on `±X mask`;
      no enforcement/persistence/initial query. Free on channel reset.
- [x] Reversal semantics preserved exactly per D-CHM6's per-class table:
      flag bounce/reversal stays strictly chanmode-gated (intent — pinned
      by the step-0 negative test); prefix unconditional; key/limit
      restore previous; b/e/I own settings; generic LIST never bounced.
- [x] Remove the gotmode sanity-check warnings (the
      `strchr("behIklov"…)` block) — superseded by the dispatch (D-OOS3).
- [x] New tests (B5): `+S` flag tracked + `bind mode +S|`; `+j 3:5`
      tracked with arg; quiet-LIST `+q mask` → generic store, **no**
      `CHANQUIET` (assert `getchanmode` has no `q`), `bind mode +q mask`;
      flag-q net inverse; server-set non-classic flag in `chanmode` (e.g.
      `chanmode +ntS`, server `-S`) reversed — generic protection reaches
      non-classic flags; the step-0 negative (unprotected flag never
      bounced) still green.
- [x] Sequencing note: this step pulled forward narrow read-only
      `getchanmodes`/`chanmodelist` Tcl commands and a FLAG-only generic
      protection bitset (`mode_pls_prot_generic`/`mode_mns_prot_generic`) so
      B5 could assert state and non-classic flag reversal directly. Steps 8
      and 10 complete those surfaces rather than introducing them from zero.

### Gate 5

- [x] Build clean; **entire A-suite green unmodified** — this gate is the
      whole point of step 0. Pay special attention to A2 (bind ordering),
      A5/A13–A17 (policy merge), A22/A23 (preamble/bounce).
- [x] B5 green.
- [x] Committed.

---

## Step 6 — Outbound queue rework + `pushmode` validation

Implements D-Q1/D-Q2/D-Q3/D-Q4/D-Q5, D-TCL2. Removes the
`cmode[]`/`pls`/`mns` limitations and the 6-modes-per-line cap.

- [x] Replace the legacy queue block in `chanset_t` (src/chan.h):
      `pls/mns/key/rmkey/limit/bytes/compat/cmode[]` are **removed**
      (D-Q5), along with `MODES_PER_LINE_MAX`; new fixed 32-entry array of
      `{char sign; char modechar; char *arg;}`. Update channels.mod
      `clear_channel`/`expmem` (channels.c:416, 826) to walk the new
      array. UPGRADING note for the removed fields/macro.
- [x] Honor server MODES up to 32 (D-Q4): widen the `irc_isupport()`
      MODES clamp to 1..32 (default 3); clamp `modesperline` at point of
      use instead of the per-second re-clamp in `flush_modes`
      (irc.c:1051) — delete the re-clamp.
- [x] Byte-budget flushing: flush when entry count reaches `modesperline`
      *or* the projected line would exceed ~450 bytes (also fixes the
      latent 6-long-masks truncation bug).
- [x] Rewrite `real_add_mode()` as a validating wrapper preserving the
      exported `add_mode(chan, plus, mode, op)` signature: `modecharinfo`
      arg-validation (`MODE_HAS_SET_ARG`/`MODE_HAS_UNSET_ARG`) →
      `can_set_mode()` → per-class dedup (prefix via `sentplus/sentminus`
      + state; b/e/I via `ischan*` + `max_*` caps as today; flags drop
      exact queued duplicates) → append.
- [x] Rewrite `flush_mode()`: walk in insertion order, emit sign changes
      inline (`+b-b+b a b c`), split lines at `modesperline` modes /
      buffer limits; k/l are ordinary entries (drop `include_lk`
      special-casing from the line-budget); preserve the
      `prevent_mixing` e/I flush-barrier behaviour (D-Q3).
- [x] `tcl_pushmode`: surface validation failures as Tcl errors (unknown
      mode, missing/excess arg) — D-TCL2.
- [x] Audit internal `add_mode` callers that relied on the old
      "-list before +list" grouping; reorder call sites if needed so wire
      semantics are unchanged.
- [x] New tests (B6): `pushmode +j 3:5` on the wire; push-order
      `+b-b+b a b c` single line; `pushmode +Z` (unknown) → Tcl error;
      `pushmode +j` (missing arg) → error; byte-budget split with long
      masks (line < 512 bytes always).
- [x] **Rewrite B0** (sanctioned, D-category): `MODES=20` is now honored —
      assert >6 modes per line where the byte budget allows, masks
      complete across lines.

### Gate 6

- [x] Build clean; A6–A9 (queue characterization) green **unmodified** —
      identical wire output from the new queue (A9's MODES=4 split is the
      canary). Full suite green.
- [x] B6 green, rewritten B0 green.
- [x] Grep gate: `grep -rn "MODES_PER_LINE_MAX\|->cmode\|->pls\b\|->mns\b"
      src/` empty.
- [x] Committed.

---

## Step 7 — `getchanmode`/`got324` from the new store

Implements D-CHM5. The sanctioned A-test migration happens here.

- [ ] Rewrite `getchanmode()` to render from `modeflags`/`modeargs`:
      flags in bitset order (`a-z`, `A-Z`, `0-9`), args appended in the
      same order. Same `+flags arg arg` shape.
- [ ] Rewrite `got324()` generically (type dispatch, like gotmode but
      state-only + recheck trigger); preserve the `key=*` →
      `CHAN_ASKEDMODES` quirk (A27) and the ASKEDMODES→
      `recheck_channel_modes` tail. Delete the per-letter chain and its
      sanity warnings.
- [ ] **Migrate** (ARCHITECTURE.md §C) the two existing 324 skip/warn
      tests to assert tracking: unknown-to-legacy modes now appear in
      `getchanmode`; the conflict-warning assertions are removed. Note
      the behaviour change in the PR description draft.
- [ ] New tests (B7): 324 with arbitrary advertised flag/param modes
      appears in `getchanmode` (set-membership + args).

### Gate 7

- [ ] Build clean; full suite green with **only** the two sanctioned test
      migrations changed (diff of `tests/` must show nothing else).
- [ ] A1 (set-membership form) passes against the new ordering.
- [ ] Committed.

---

## Step 8 — channels.mod: verbatim chanmode + generic protection

Implements D-TCL3, D-CHM3, D-CHM4, D-ISU-adjacent lazy re-parse (D11).

- [ ] Append `char *chanmode_verbatim` (or sized buffer) to `chanset_t`;
      `channel set/add … chanmode` and `.chanset` store verbatim after
      validating shape (`±[a-zA-Z0-9]` words + args); warn on letters
      unknown to current `modecharinfo`. `channel get`/chanfile write
      return/persist the verbatim string.
- [ ] Parse verbatim → generic desired-mode store (uint64 pls/mns + args
      for LIMIT/KEY-type letters), extending/replacing step 5's FLAG-only
      `mode_pls_prot_generic`/`mode_mns_prot_generic` bridge. Classic
      letters additionally maintain `mode_pls_prot`/`mode_mns_prot`/
      `limit_prot`/`key_prot` mirrors (D-CHM4). LIST/PREFIX-type letters:
      warn + ignore (D-CHM3).
- [ ] Re-parse lazily: on connect (post-005) and on CHANMODES/PREFIX
      isupport changes (hook the existing irc.mod isupport bind).
- [ ] Rewrite `recheck_channel_modes()` generically over the desired
      store (flags + parameterized enforce). Convert the `gotmode`
      enforcement reads from the step-5 bridge/legacy fields to the final
      store.
- [ ] Add `chanmode_prot_arg(chan, mode)` accessor; convert all internal
      `key_prot`/`limit_prot` readers — including every JOIN-key idiom
      site (`grep -rn "key_prot" src/`) in irc.mod, server.mod,
      channels.mod — to accessors. Fields stay, writers keep them
      mirrored.
- [ ] New tests (B8): `.chanset #c chanmode +zS` pre-connect → stored
      verbatim + warned, `channel get` returns it; post-connect (z/S
      advertised) both enforced; `+j 3:5` param enforcement; LIST/PREFIX
      letter in chanmode → warn+ignore; reconnect with different
      CHANMODES re-derives enforcement.

### Gate 8

- [ ] Build clean; A10–A12, A25 (enforcement + round-trip) green
      **unmodified**; full suite green.
- [ ] B8 green. Grep gate: no direct `chan->key_prot`/`chan->limit_prot`
      *reads* outside channels.mod's mirror-writer and the accessor
      (`grep -rn "key_prot\|limit_prot" src/` — writers/mirrors only).
- [ ] Committed.

---

## Step 9 — ISUPPORT persistence in the userfile

Implements D-ISU2..5.

- [ ] server.mod: accumulate the verbatim ISUPPORT string (all 005 lines'
      token text concatenated in arrival order, no dedup; reset per
      connection) and publish it into a single core global (declare in
      core, e.g. alongside other server-shared globals; document "may
      change").
- [ ] `write_userfile()` (src/userrec.c): after the `#4v:` header, emit
      `#isupport <string>` when the global is non-empty. **No other write
      path** (preserves `-m`: `userlist == NULL` early-return already
      guards).
- [ ] `readuserfile()` (src/users.c): when `bu == userlist` (startup/
      reload only — never share transfers), capture a leading
      `#isupport ` comment line into the global. Unknown `#` lines remain
      skipped as today.
- [ ] server.mod: on `userfile-loaded`, apply the captured string through
      the isupport-default machinery (precedence: user-set
      `isupport-default` Tcl var > persisted > compiled default), which
      re-fires the binds and populates `modecharinfo` pre-connect.
- [ ] New tests (B9): 005 → `.save` → userfile contains the verbatim
      header (multi-005 concatenated in order); restart on that userfile
      with no 005 → `chanmodeinfo`/pushmode validation reflect persisted
      modes pre-connect; old-format userfile (no header) loads cleanly;
      `isupport-default` set in conf wins over the persisted line.

### Gate 9

- [ ] Build clean; full suite green (notably the existing userfile tests
      and `test_tcl_*` suites — the userfile format change must be
      invisible to them).
- [ ] B9 green. Manual check: a 1.10-built eggdrop (or `develop` build)
      loads a userfile containing the `#isupport` line without complaint
      (comment-skip verified).
- [ ] Committed.

---

## Step 10 — New Tcl introspection commands + docs

Implements D-TCL1; closes the loop for assertions deferred from steps 4–5.

- [ ] Complete in tclirc.c (names final per ARCHITECTURE.md):
      `chanmodeinfo <mode-or-prefixchar>` (dict: type/prefix/rank),
      `isprefix`/`wasprefix`/`isprefixatleast <mode-or-prefixchar> <nick>
      [chan]`, plus any API polish/error handling needed for the step-5
      `getchanmodes <chan>` and `chanmodelist <chan> <mode>` commands.
- [ ] Extend B4/B5 tests to assert through these commands where not already
      covered by the step-5 `getchanmodes`/`chanmodelist` assertions.
- [ ] B10: direct bridge tests for each command (arg handling, error
      cases, unknown mode/nick/chan).
- [ ] Docs: `doc/sphinx_source/using/tcl-commands.rst` (new commands,
      `bind mode` fires for all modes, `pushmode` error behaviour,
      `getchanmode` ordering note); UPGRADING/NEWS entries (`opchars`
      deprecated, `NOHALFOPS_MODES` removed, multi-prefix negotiated,
      userfile `#isupport` header, behaviour fixes from B3/section C).
- [ ] Update ARCHITECTURE.md if any decision drifted during
      implementation (it must stay authoritative).

### Gate 10 (= PR gate)

- [ ] `make test` (full coverage build + suite) green from scratch.
- [ ] `ruff check` / `ty check` clean.
- [ ] Coverage spot-check: new dispatch/accessor/queue code exercised
      (per-file `gcov` on mode.c/chan.c).
- [ ] Full-suite diff review: the only modified pre-existing tests are
      the sanctioned ones (steps 2 and 7).
- [ ] PR description drafted: summary, decision links into
      ARCHITECTURE.md, **explicit behaviour changes** (literal-o
      `isop`/`opchars` removal, halfop `+q` on quiet-LIST nets, 324
      unknown modes now tracked, `pushmode` errors, flush ordering,
      userfile header), accepted test gaps (share.mod, msgcmds.c,
      flood-deop) with mitigations.
- [ ] PR opened against `develop` from `arbmodes3`.
