# Arbitrary channel modes (arbmodes3)

Design and implementation plan for making Eggdrop's channel mode handling
ISUPPORT-driven. Continuation of `feature/arbchanmodes2` (merged to
`develop`), which introduced `modecharinfo[256]` (populated from 005
`CHANMODES`/`PREFIX`, see `src/mod/irc.mod/irc.h:36-59`,
`chan.c:process_chanmodes()`/`process_prefix()`) plus sanity checks, but did
not change any of Eggdrop's hardcoded assumptions about what mode letters
mean.

Branch: `arbmodes3`, single PR at the end. Tests live in `tests/` (pytest
harness, see `tests/README.md`).

The **Decisions** section is the authoritative record; it incorporates a
detailed design review (the "grill", Q1–Q12 below) and supersedes any
earlier shorthand where they differ.

## Goals

1. `pushmode`/`add_mode` can send any mode the IRCd understands, with
   correct argument handling per `modecharinfo` type.
2. Channel modes tracked generically: `a-zA-Z0-9` as a bitset plus a stored
   argument for every set mode that takes one, behind accessor functions
   that keep the legacy fields (`chan->channel.mode`, `.key`,
   `.maxmembers`) in sync.
3. Member prefixes tracked generically (owner/admin/etc., not just o/h/v),
   behind accessors that keep `memberlist.flags` (`CHANOP`/`CHANHALFOP`/
   `CHANVOICE` and the `WAS*`/`SENT*`/`FAKE*` bits) in sync.
4. `bind mode` fires for *all* mode changes, not only the historic letters.
5. Simplified, **readable** code: generic `gotmode` dispatch, a single
   parameterized op+halfop policy pair, and removal of the per-letter flag
   switch / `opchars` / `NOHALFOPS_MODES`.
6. New generic *tracking* for previously-unknown list modes (quiet, …),
   handled uniformly and separately from the legacy b/e/I machinery.

**Backwards compatibility is the highest priority**, at the source level.
Third-party C modules that read `memberlist.flags`, `chan->channel.mode`,
`chan->key_prot`, etc. must keep working after a recompile; Tcl scripts must
keep working unchanged. New struct fields are appended (binary/.so compat is
not preserved; source compat is). **Optimize for readability, not lines of
code** — keeping two near-identical-but-genuinely-different functions
separate is preferred over a merged function with a branch-snarl.

## Current state: where mode semantics are hardcoded

Condensed inventory; these are the sites the steps below convert.

### Flag modes — `chan->channel.mode` bitfield ↔ 20 fixed letters

`ipsmtnlkaqcRMCrDuNdT` ↔ `CHANINV..CHANNOAMSG` (`src/chan.h:138-157`),
maintained in five independent letter tables:

- `mode.c:gotmode()` — per-letter switch on live MODE; unknown letters are
  silently skipped (no state, no bind).
- `chan.c:got324()` — per-letter chain on the 324 reply, plus the
  arbchanmodes2 sanity checks (`"kl"` / `"ipsmcCRMrDuNTdtnaq"`).
- `chan.c:getchanmode()` — bit→letter serialization, fixed order.
- `chan.c:recheck_channel_modes()` — one if-pair per letter enforcing
  `mode_pls_prot`/`mode_mns_prot`.
- `channels.mod/channels.c:set_mode_protect()/get_mode_protect()` —
  `chanmode` setting parse/serialize; unknown letters silently dropped.

Outside consumers: `channel_hidden`/`channel_optopic` macros
(`src/chan.h:262-263`), `mode_pls_prot`/`mns_prot`/`limit_prot`/`key_prot`
in `chanset_t`, Tcl `channel get/set chanmode`, default chanmode `"nt"`.

### Prefix modes — `memberlist.flags` assumes exactly o/h/v

- Flag clusters `CHANOP/SENTOP/SENTDEOP/FAKEOP/WASOP` (×3 for o/h/v,
  `src/chan.h:58-97`). Other prefixes (`~q` owner, `&a` admin) have no
  representation.
- `mode.c:real_add_mode()` — SENT*/dup suppression only for o/h/v.
- `mode.c:got_op/got_deop/got_halfop/got_dehalfop` + inline `'v'` case.
- `chan.c:got352or4()` (WHO/WHOX) — `opchars` (Tcl variable, default `"@"`),
  hardcoded `'%'`, `'+'`, `'G'`, `botflag005`.
- `chan.c:got353()` (NAMES) — only runs under `userhost-in-names`; strips
  prefix chars to recover the bare nick and passes `""` flags (prefix info
  intentionally discarded; userhost is the point).
- `HALFOP_CANTDOMODE`/`NOHALFOPS_MODES "ahoq"` (`src/chan.h:33-42`),
  ~30 call sites with literal mode chars.
- Tcl `isop/ishalfop/isvoice/wasop/washalfop`.

### Key/limit special-casing

Dedicated fields: `channel.key`/`channel.maxmembers` (current state),
`key_prot[121]`/`limit_prot` (desired), `chanset_t.key/rmkey/limit` +
`include_lk` (outbound queue), hardcoded `'k'`/`'l'` emission in
`mode.c:flush_mode()`, the Undernet `key=*` → `CHAN_ASKEDMODES` hack.
Note `key_prot` is also the **JOIN password** (`key = chan->channel.key[0] ?
chan->channel.key : chan->key_prot`) used in ~15 sites across irc.mod,
server.mod (`servmsg.c:398/441`), channels.mod.

### List modes — exactly b/e/I, triplicated

Three `masklist`s in `chan_t`, three `maskrec` lists in `chanset_t`
(userfile + share.mod), numerics 367/368, 348/349, 346/347, six
`got_(un)ban/exempt/invite` handlers, `parse_maxlist()` crediting only
b/e/I, ~40 literal `add_mode(chan, ±, 'b'/'e'/'I', …)` calls.

### Outbound mode queue

`chanset_t.cmode[].type` is a bitmask (`CHOP/BAN/VOICE/EXEMPT/INVITE/
CHHOP`) — only six letters can carry a parameter at all. Every other
parameterized mode pushed via `add_mode()` falls into `pls[21]`/`mns[21]`
and **loses its argument**. The core `pushmode` blocker. `flush_mode()`
groups `-b/-e/-I` before `+b/+e/+I` and emits k/l from special slots.
`chan->compat`/`prevent_mixing` hardcodes the e/I-vs-rest server split.

### Known pre-existing bug (fix in step 0)

`irc_isupport()` parses `MODES` up to 64 into `modesperline`
(`chan.c:3056`) but `cmode[]` is `MODES_PER_LINE_MAX` = 6
(`src/chan.h:39,210`) and `real_add_mode()`/`flush_mode()` loop
`i < modesperline` over it → out-of-bounds access on servers advertising
`MODES>6` (InspIRCd ~20). Mitigating detail found during step 0: the
per-second `flush_modes()` (irc.c) re-clamps `modesperline` to
`MODES_PER_LINE_MAX` on every tick, so the OOB window is only the
sub-second gap between 005 processing and the next tick (e.g. modes
pushed from a raw/isupport bind). Still UB — clamp at parse time
(done in step 0); the queue rework removes the fixed array and can lift
the 6-mode ceiling properly.

## Decisions

Resolved with the maintainer, including the Q1–Q12 design review. Where the
review changed an earlier call, only the final decision is recorded here.

### Member prefixes & capability

- **D-PFX1 (Q1): Policy stays literal o/h/v.** The bitch / stopnethack /
  protectops / protectfriends / revenge / flood-deop policy logic applies
  only to `o`/`h`/`v`. Higher prefixes (owner/admin/…) get **state update +
  `bind mode` only** — no built-in policy; scripts implement their own. The
  resulting asymmetry (e.g. `+bitch` doesn't auto-de-owner) is a deliberate,
  documented gap for a later design pass.
- **D-PFX2 (Q1/Q9 revised): Legacy `CHANOP`/`isop`/`me_op` are literal
  `o` only.** Higher prefixes do **not** imply op. The old "higher modes
  count as op" behaviour existed only via the `opchars` hack and is removed
  along with `opchars`. `CHANHALFOP`/`CHANVOICE` mirror literal `h`/`v`.
- **D-PFX3 (Q2): `me_op`/`me_halfop`/`me_voice` stay literal** (exported via
  `irc_funcs`, meaning unchanged). New rank-based capability lives in a
  separate `can_set_mode()`.
- **D-PFX4 (Q2/Q3): `can_set_mode(chan, mode)` is the rank-based gate**,
  computing the bot's best rank from its own prefix bitset directly (not via
  `CHANOP`):
  - bot at highest advertised prefix → may set anything;
  - target is a **prefix** mode of rank `r` → allowed iff `r` is strictly
    below the bot's rank, **except** `o` may set/unset `o`;
  - target is a **non-prefix** mode → allowed iff the bot is at-least-halfop
    (`me_op || me_halfop`), preserving today's gate.
  Replaces `HALFOP_CANTDOMODE`/`HALFOP_CANDOMODE`. The per-mode
  `NOHALFOPS_MODES "ahoq"` string is **removed** (subsumed by rank for a/h/o;
  the `q`-as-quiet case is *fixed* — a halfop may now set quiet, a list
  mode). The coarse `NO_HALFOP_CHANMODES` ifdef is **kept** for non-rank-0
  bots; rank 0 still may set anything per this rule.
- **D-PFX5 (Q9): WHO/WHOX is the authoritative prefix source.** 354 (WHOX,
  already preferred via `use_354`) reports **all** prefixes even without
  multi-prefix; legacy 352 reports only the highest. Generalize
  `got352or4()`'s flags-field parsing from `@`/`%`/`+`/`opchars` to the
  full PREFIX set; remove `opchars` (Tcl variable accepted-but-ignored with
  a once-per-irc.mod-load deprecation warning on Tcl writes this release).
  **NAMES (353) logic is unchanged** —
  stays gated on `userhost-in-names`, still passes `""` flags, does not
  contribute prefix state (so the generalized `got352or4` clears prefix
  bits on that path exactly as it clears CHANOP today: zero behaviour
  change).
- **D-PFX6 (Q9, mechanism grill-2): Negotiate `multi-prefix`** where the
  server offers it. server.mod has no module-cap-registration API and we are
  not adding one for a single cap: the request lives in server.mod's CAP LS
  handler like sasl/account-notify, gated on a new server.mod-local
  `multi-prefix` Tcl config var **defaulting to 1** (precedent:
  `extended-join` defaults on despite changing raw `JOIN` lines far more
  invasively than `@+nick` in NAMES; the var is the script-compat escape
  hatch, noted in UPGRADING). The var is not exported through the C module
  API unless a later step strictly needs it. irc.mod's 352/353/354 parsers
  strip **all** leading current PREFIX chars unconditionally — correct under
  both cap states since nicks cannot start with a prefix char under the
  advertised PREFIX set; they also keep a conservative legacy fallback for
  `@`/`%`/`+` when the prefix table is missing or incomplete. Improves the
  352 and NAMES paths; 354 is already complete.
- **D-PFX8 (grill-2): Member prefix bits are rank-indexed (`1 << rank`),
  resynced on PREFIX change.** A mid-session 005 PREFIX re-announcement
  (rare; e.g. InspIRCd module load) would silently remap every member's held
  bits. When `update_chanmodes(is_prefix=1)` results in an effectively
  different prefix set/order: clear `prefixmodes`/`wasprefix`/sent bits (and
  the legacy mirror bits) on all members and `reset_chan_info(chan,
  CHAN_RESETWHO)` each active channel — WHOX rebuilds authoritative state.
  Identical replays (including step-1 isupport replay) do not reset members.
  Per-letter stable bit allocation was rejected: more bookkeeping, and rank
  shifts under `can_set_mode` anyway, so it only half-solves the problem.
- **D-PFX9 (grill-2): The 8-prefix cap degrades tracking, never parsing.**
  PREFIX letters beyond `MAX_PREFIX_MODES` keep a full `modecharinfo` entry
  (type=PREFIX, prefix char, rank — uint8 positions beyond 7 are fine), so
  arg consumption, `bind mode`, and `can_set_mode` stay correct; only the
  per-member bit is unavailable (`isprefix`-family returns 0; log once at
  PREFIX-parse time). Fully dropping the letter would desync the parser —
  one `+X nick` and every later arg in the line is misattributed.
- **D-PFX7 (Q1/Q10): Merge op+halfop policy into one parameterized
  `got_prefixmode`/`got_deprefixmode` pair** (true structural twins:
  CHANOP↔CHANHALFOP, protectops↔protecthalfops, autoop↔autohalfop, shared
  stopnethack). **Keep voice separate** (autovoice/quiet, structurally
  different). Higher prefixes are policy-free. Do **not** force a single
  all-modes function.

### Channel modes (flag/key/limit) & protection

- **D-CHM1 (D4): `chan_t` stores the argument for every set mode that takes
  one** (KEY- and LIMIT-type), not just k/l, behind accessors.
- **D-CHM2: Type-gated legacy mirror.** A legacy bit/field is updated only
  when the letter's `modecharinfo` type matches Eggdrop's historic
  assumption. Libera `+q` (list type) must **not** set `CHANQUIET` (a flag
  bit); `k` mirrors `CHANKEY`/`channel.key` only while KEY-typed; `l`
  mirrors `channel.maxmembers` only while LIMIT-typed.
- **D-CHM3 (Q7): Protection generalizes to flag and parameterized modes.**
  `chanmode` protection enforces/rejects any FLAG-type letter (classic or
  new, e.g. `+S`) and any LIMIT/KEY-type letter *with its argument* (e.g.
  `+j 3:5` like `+l 10`). LIST/PREFIX-type letters in a `chanmode` string
  are **warned and ignored** (no standing-enforce semantics).
  Implementation note: step 5 pulled forward a minimal FLAG-only generic
  protection bitset so live `gotmode` reversal can cover non-classic flags;
  step 8 replaces/extends that into the final verbatim, type-aware desired
  store with parameter arguments.
- **D-CHM4 (Q8): Keep `key_prot`/`limit_prot` (and `mode_pls_prot`/
  `mode_mns_prot`) as compat mirrors** — source compat is most important.
  The new generic desired-mode store is the source of truth; the legacy
  fields are maintained for the classic letters; **all internal call sites
  (incl. the JOIN-key idiom) are refactored to unified accessors**
  (`chanmode_prot_arg(chan, mode)` etc.).
- **D-CHM5 (Q13/old): `getchanmode` ordering may change** (not API-breaking):
  bitset order `a-z`, `A-Z`, `0-9`, args appended in the same order.
- **D-CHM6 (grill-2): Flag-mode reversal stays strictly chanmode-gated —
  this is intent, not accident.** Step 0 found that under `reversing`
  (bounce-modes / fakeop / desync) a flag mode is only reversed when it is
  also chanmode-protected in the corresponding direction (mode.c:1314
  reduces to the same protection test as the non-reversing branch, minus
  the master exemption). Rationale: only a `chanmode` entry expresses the
  user's intent to enforce; other flag modes are irrelevant. The generic
  dispatch preserves this exactly. Per-class `reversing` table: PREFIX —
  unconditional reverse; FLAG — only if chanmode-protected; KEY/LIMIT —
  restore previous value; b/e/I — own bounce settings; generic LIST —
  **never bounced** (no policy basis; future per-type setting alongside the
  generic list-add command). Now contractual: pinned by a negative
  characterization test (server `+s` under bounce-modes, `s` not in
  chanmode → not reversed).
- **D-CHM7 (grill-2): Enforcement gates on an explicit "modes known"
  state.** `CHAN_ASKEDMODES` means "info incomplete, re-ask at next
  recheck" (Undernet `+k *`), but `reset_chan_info` *clears* it when
  sending the join-time `MODE` query (irc.c:461), so end-of-WHO enforcement
  can run against `channel.mode == 0` and push the whole chanmode blind —
  every step-0 test needed a fake 324 reply to keep wires clean. Fix: the
  new tracking struct carries a modes-known flag (false at join/reset, set
  by got324); `recheck_channel_modes` and the generic arg-mode protection
  additionally gate on it. `CHAN_ASKEDMODES` keeps its re-ask semantics
  untouched. `got324` keeps running recheck even while CHAN_PEND, so
  enforcement timing on normal networks (324 before 315) is byte-identical.
  D-test: delay 324 past 315 → no push before 324, correct push after.

### ISUPPORT availability & persistence

- **D-ISU1 (Q4): Seed `modecharinfo` so it is never empty.** It is parsed
  from the (possibly persisted, see below) ISUPPORT defaults before connect;
  `pushmode` therefore errors only on genuinely-unknown letters, not on
  standard modes used pre-connect.
- **D-ISU6 (grill-2): Generic replay fixes *all* stale-on-load isupport
  consumers, not just CHANMODES/PREFIX.** The isupport store is push-only:
  values are delivered once, at change time, to whoever is bound at that
  moment (isupport.c:410 documents the resulting hole). After a
  mid-connection irc.mod reload, `use_354`, `modesperline`,
  `max_exempts/max_bans/max_modes`, `botflag005` and `modecharinfo` all
  silently revert to compiled defaults. Fix: (a) parse the default ISUPPORT
  string eagerly in `isupport_init()` (it was deferred to preconnect *only*
  because of the bind-ordering problem this replay removes; `preconnect()`
  keeps re-applying the `isupport-default` Tcl var before each connect);
  (b) export `isupport_replay()` (appended `server_funcs` slot) which walks
  `isupport_list` and re-fires the bind table for every record with an
  effective value; (c) every consumer module calls it in its `_start` right
  after `add_builtins(H_isupport, ...)`. Contract change, documented in
  UPGRADING: **isupport binds must be idempotent** — they may be re-fired
  with unchanged values when a module loads. All existing handlers already
  are.
- **D-ISU2 (Q5/Q6): Persist the last-seen ISUPPORT to the userfile.** Store
  the **whole verbatim** ISUPPORT string — all 005 lines concatenated in
  arrival order, **no dedup** — as a single `#`-prefixed comment header line
  right after `#4v:` in `write_userfile`. Old parsers skip `#`/`;` lines
  (`users.c:715`), so it is backward/forward compatible.
- **D-ISU3 (Q6): Lightweight, no hooks.** A single core global holds the
  string; server.mod updates it on 005 and `write_userfile` emits it; on
  read, core captures the line into the global; server.mod applies it on
  `userfile-loaded`, feeding it through the existing `isupport-default`
  machinery so `modecharinfo` populates before connect. Precedence:
  user-set `isupport-default` > persisted > compiled default. The ISUPPORT
  string itself is the extension point for any future need.
- **D-ISU4 (Q5): No leak across a botnet.** Parse the persisted line only on
  the bot's own startup load (`bu == userlist`, `users.c:688`), never on a
  share.mod file transfer (`share.c:1928`).
- **D-ISU5 (Q5/-m): `-m` stays enforced.** The line is emitted only by
  `write_userfile`, which already returns early when `userlist == NULL`
  (`userrec.c:642`). No separate "save ISUPPORT on 005" write path is added,
  so no ownerless userfile can be created during `-m`.

### Outbound queue

- **D-Q1 (D20): Flush order = push order.** `flush_mode()` does not reorder
  by type; the `add_mode()` call sequence is preserved and `+`/`-` may
  interleave within one line (`MODE #c +b-b+b a!*@* b!*@* c!*@*`). Drops the
  historic "-b before +b" grouping; overlap ordering becomes caller-
  controlled (internal callers audited to emit intended order).
- **D-Q2: Generalized queue carries `(sign, mode, arg)`** so any
  parameterized mode round-trips (D16: args are single space-delimited
  tokens, may contain `:`/`#` etc.). `add_mode()` keeps its exported
  signature and becomes a validating wrapper (`modecharinfo` type check →
  `can_set_mode` → per-class dedup → append). k/l lose their special slots.
- **D-Q3 (D14): `prevent_mixing`/`compat` behaviour kept as-is** — the e/I
  flush-barrier is preserved; it splits lines, it does not reorder within a
  line, so it coexists with D-Q1.
- **D-Q4 (grill-2): Honor server MODES up to a sanity cap of 32** (seen in
  the wild) instead of clamping to 6. The new queue is a fixed array of 32
  `{char sign; char modechar; char *arg;}` entries in `chanset_t` (defined
  openly in src/chan.h; `arg` nmalloc'd by irc.mod, walked directly by
  channels.mod `clear_channel`/`expmem` as today — no opaque-pointer
  machinery). `modesperline` governs the flush threshold only, clamped
  1..32 **at parse time and at point of use** (the per-second re-clamp in
  `flush_modes` dies). Flushing is additionally **byte-budget-aware**:
  flush when the count reaches `modesperline` *or* the projected line would
  exceed ~450 bytes — 12-32 ban masks blow the 512-byte line limit, and
  this also fixes the latent truncation bug where 6 long masks already can.
  D-tests: A9 (MODES=4 split) unchanged; B0's MODES=20-clamped pin becomes
  an intentional change (honored, byte-split); new test for byte-budget
  splitting with long masks.
- **D-Q5 (grill-2): The legacy queue block is removed outright** —
  `pls/mns/key/rmkey/limit/bytes/compat/cmode[]` leave `chanset_t` and
  `MODES_PER_LINE_MAX` leaves chan.h (out-of-irc.mod users are only
  channels.mod expmem/clear_channel, both in-tree). Explicit deviation from
  the keep-fields-as-mirrors rule: this is transient outbound state, not
  observable channel state; third-party code reading it has no defensible
  use. Source-compat note in UPGRADING.

### List modes

- **D-LST1 (Q11): b/e/I stay separate and unchanged.** Keep
  `got_ban/got_unban/got_exempt/got_unexempt/got_invite/got_uninvite` and
  all their machinery (masklists, userfile, sticky/dynamic, enforce, kick).
  They are reached through the new generic dispatch (so the arg handling is
  unified and `bind mode` fires) but are not merged — they encode genuinely
  different policy.
- **D-LST2 (Q12): New generic tracking for non-b/e/I list modes** (quiet,
  …), handled uniformly: a new per-channel store (mode char → list of
  `{mask, who, time}`), updated from live `+X mask`/`-X mask`, firing
  `bind mode`, settable via `pushmode`, exposed read-only via a new Tcl
  command (e.g. `chanmodelist <chan> <mode>`). **No enforcement, no
  persistence, no sticky/dynamic, no initial-list query, no bouncing
  (D-CHM6)** in this branch.

### Tcl / introspection

- **D-TCL1 (Q23/old): New introspection commands in scope** (names: my
  call): `chanmodeinfo <mode-or-prefixchar>` (type/prefix/rank),
  `getchanmodes <chan>` (set non-list modes → arg), `isprefix` /
  `wasprefix` / `isprefixatleast <mode-or-prefixchar> <nick> [chan]`,
  `chanmodelist <chan> <mode>` (generic list masks).
  Implementation note: step 5 added `getchanmodes` and `chanmodelist` early
  as narrow read-only assertions for B5. Step 10 completes the remaining
  commands, error coverage, docs, and any API polish those early commands
  still need.
- **D-TCL2 (D21): `pushmode` errors to Tcl** on a mode unknown to
  `modecharinfo` or an argument-count mismatch for the mode's type. No
  silent drop, no pass-through.
- **D-TCL3 (D22): `chanmode` stored verbatim** in channels.mod and the
  chanfile; parsed against live `modecharinfo`; `.chanset chanmode` accepts
  any `±[a-zA-Z0-9]`(+args) string but **warns** about letters it cannot yet
  classify. With D-ISU2 the early window shrinks (defaults/persisted seed the
  table), so verbatim deferral only matters for still-unknown letters.

### Out of scope / unchanged

- **D-OOS1 (D18): twitch.mod out of scope**; its fake-WHO path stays inert.
- **D-OOS2 (D17): share.mod/userfile records for non-b/e/I list modes
  deferred.**
- **D-OOS3 (D19): arbchanmodes2 sanity-check warnings** removed only in the
  step that makes the corresponding consumer table-driven.

## Target design

### `mode_info_t` (irc.h)

```c
typedef struct mode_info {
  uint8_t type;         /* mode_type_t value                         */
  uint8_t rank;         /* PREFIX position, 0 = highest; sentinel for
                           non-prefix modes. Doubles as bit index into the
                           per-member prefix bitsets (max MAX_PREFIX_MODES). */
  char prefix;          /* prefix char for MODETYPE_PREFIX, else 0   */
} mode_info_t;
```

Helpers: `MODE_RANK(c)`, `mode_by_prefixchar(char)`, `mode_to_index(c)`
(`a-z`→0-25, `A-Z`→26-51, `0-9`→52-61, else -1). PREFIX entries beyond
`MAX_PREFIX_MODES` (8) keep full `modecharinfo` entries (parsing, bind,
rank stay correct) but get no per-member bit — see D-PFX9. New bitfield
fields use `uint8_t` throughout. The field order is intentionally
no-padding/byte-comparable; temporary `mode_info_t` arrays explicitly write
all fields, including invalid entries, so effective PREFIX changes can be
detected with `memcmp`.

### `chan_t` additions (appended; "may change — use accessors")

```c
uint64_t modeflags;        /* bit mode_to_index(c) set = mode c active   */
char    *modeargs[62];     /* arg for set arg-taking modes, else NULL    */
chanmode_list *modelists;  /* non-b/e/I LIST modes                       */
```

Accessors (irc.mod): `chanmode_set/unset/isset/getarg/clear`. They maintain
the type-gated legacy mirror (`channel.mode` bit, `channel.key`,
`channel.maxmembers`). `modeflags` also reserves an internal high bit for
the D-CHM7 "modes known" state; only bits 0..61 are mode-letter slots.
`modelists` is the live, read-only generic LIST store from D-LST2; it is
not persisted and has no enforcement or initial-list query. Because 324 is
a snapshot of standing non-list channel modes, a 324 refresh clears and
rebuilds only `modeflags`/`modeargs` state; it must not clear `modelists`,
which is delta-tracked from live MODE/list numerics.

### `chanset_t` additions (appended; "may change — use accessors")

```c
uint64_t mode_pls_prot_generic; /* desired + non-classic FLAG modes */
uint64_t mode_mns_prot_generic; /* desired - non-classic FLAG modes */
```

These are the step-5 FLAG-only protection bridge for non-classic modes
(`+S`/`-S` style). Step 8's verbatim chanmode work should replace or
extend this into the final generic desired-mode store, including KEY/LIMIT
arguments and type-aware LIST/PREFIX rejection, while keeping the legacy
`mode_pls_prot`/`mode_mns_prot` mirrors correct for classic letters.

### `memberlist` additions (appended; "may change — use accessors")

```c
uint8_t prefixmodes;  /* bit (1<<rank) per held prefix mode        */
uint8_t wasprefix;    /* held before split / for bind mode         */
uint8_t sentplus;     /* +mode already queued (anti-loop)          */
uint8_t sentminus;    /* -mode already queued                      */
```

Internal irc.mod accessors stay private/static unless a later step strictly
needs C module API exposure. Accessors take a mode letter (`a-zA-Z0-9`) or a
prefix char (anything else, resolved through `mode_by_prefixchar()`):
`member_has_prefixmode`, `member_had_prefixmode`,
`member_has_prefixmode_atleast`, current/was setters, and sentplus/sentminus
readers/setters. They no-op silently for ranks beyond `MAX_PREFIX_MODES`.
They mirror the legacy `CHANOP`(literal o)/`CHANHALFOP`/`CHANVOICE` and
`WAS*`/`SENT*` bits for o/h/v. `wasprefix` follows the `wasop` bind-time
contract: current state changes before `bind mode`, previous-state bits
change after the bind.

### Capability `can_set_mode(chan, mode)` — see D-PFX4.

### Outbound queue — see D-Q1/D-Q2/D-Q4/D-Q5. New ordered `(sign, mode,
arg)` queue: fixed 32-entry array in `chanset_t`, replacing the legacy
`pls/mns/cmode/key/rmkey/limit/bytes/compat` fields, which are **removed**
along with `MODES_PER_LINE_MAX`. Flush at `modesperline` entries (clamped
1..32 at use) or the ~450-byte line budget, whichever first.

### `gotmode()` dispatch (state-vs-bind ordering is a contract)

| Type | Order (matches today) |
|---|---|
| PREFIX | update member state → fire bind → policy (o/h/v only) |
| LIST (b/e/I) | update masklist → fire bind → policy |
| LIST (other) | update generic list store → fire bind |
| FLAG | fire bind → update channel state → enforcement |
| KEY `+` | mirror bit → fire bind → store key → got_key policy |
| KEY `-` | mirror bit → fire bind → policy → clear key |
| LIMIT `+` | update state → fire bind → enforcement |
| LIMIT `-` | fire bind → policy → clear state |

`check_tcl_mode` fires for **every** mode (args by type: prefix→nick,
list→mask, key/limit→arg with the existing `-l`→`""` quirk, flag→`""`).

### channels.mod — see D-CHM3/D-CHM4/D-TCL3. Verbatim `chanmode`, generic
desired store (uint64 pls/mns + args) as source of truth, classic mirrors
maintained, `recheck_channel_modes()` rewritten generically, lazy re-parse
on connect / CHANMODES change.

## Implementation steps

Each step is a separate commit, keeps the suite green, and adds pytest
coverage before the behaviour it protects changes (mock IRCd
`drive_registration(isupport_tokens=…)` + `tcl_bridge`).

**Step 0 — Characterization tests + MODES clamp.** Pin: `getchanmode` after
324; `bind mode` set/arg for `+o/-o/+b/+k/+l/-l`; `pushmode` known modes +
dedup; chanfile chanmode round-trip; full join handshake (NAMES/WHO/WHOX/315
sync). Fix the `modesperline`/`cmode[]` OOB (clamp `MODES` to
`MODES_PER_LINE_MAX`).

**Step 1 — `rank` + helpers + isupport replay.** Add `rank` to
`mode_info_t`, populate in `process_prefix()` (log-once past
`MAX_PREFIX_MODES`, D-PFX9); add
`mode_to_index`/`MODE_RANK`/`mode_by_prefixchar`. server.mod: parse
defaults eagerly in `isupport_init()`, export `isupport_replay()`;
`irc_start` calls it after adding binds (D-ISU1/D-ISU6) — fixes all stale
isupport state on module (re)load, not just modecharinfo. No behaviour
change otherwise.

**Step 2 — Member prefix storage.** Repack `mode_info_t` into its
no-padding byte-comparable layout. Add the bitset fields + private/static
accessors with legacy-flag mirroring (D-PFX2). Generalize `got352or4()`
flags parsing to the full PREFIX set; remove `opchars` (once-per-load
deprecation warning on Tcl writes); `multi-prefix` via server.mod CAP LS
list + server.mod-local config var default-on (D-PFX6); 352/353/354 parsers
strip leading current PREFIX chars with `@`/`%`/`+` fallback; member resync
only on effective PREFIX change (D-PFX8). NAMES remains status-blind. Convert
`gotmode` prefix cases and `real_add_mode()` SENT logic to accessors without
pulling the arbitrary outbound queue rewrite forward.

**Step 3 — Capability.** Add `can_set_mode()`; convert
`HALFOP_CANTDOMODE`/`HALFOP_CANDOMODE`/`NOHALFOPS_MODES` sites; keep
`NO_HALFOP_CHANMODES`.

**Step 4 — Channel mode storage.** Add `modeflags`/`modeargs` + accessors,
type-gated mirror (D-CHM1/D-CHM2), explicit modes-known state gating
enforcement (D-CHM7). Convert `got324`/`gotmode`/`set_key`/join/reset
writers. `getchanmode` still from legacy fields here.

**Step 5 — Generic `gotmode` dispatch + `bind mode` for all.** Rewrite the
switch to type dispatch (ordering table); merge op+halfop policy
(D-PFX7), keep voice separate; new generic list store for non-b/e/I list
modes (D-LST2); b/e/I handlers unchanged (D-LST1); remove gotmode sanity
warnings (D-OOS3). Reversal semantics preserved exactly per the per-class
table in D-CHM6 (flag bounce stays chanmode-gated; generic lists never
bounced). This step also pulled forward the minimal read-only
`getchanmodes`/`chanmodelist` Tcl commands and a FLAG-only generic
protection bitset to make B5 directly assertable.

**Step 6 — Outbound queue + `pushmode`.** New ordered 32-entry queue
(D-Q1/D-Q2/D-Q4); remove legacy queue fields + `MODES_PER_LINE_MAX`
(D-Q5); honor MODES up to 32 with byte-budget flushing, drop the
per-second re-clamp; `add_mode()` validating wrapper; `pushmode` errors to
Tcl (D-TCL2); key/limit as ordinary entries; `prevent_mixing` barrier
preserved (D-Q3).

**Step 7 — `getchanmode`/`got324` from new storage.** Render from
`modeflags`/`modeargs` (D-CHM5); delete per-letter `got324` chain + its
warnings; 324 clears standing non-list mode state only, preserving
delta-tracked generic LIST modes.

**Step 8 — channels.mod chanmode.** Verbatim storage/persistence with
warn-on-unknown (D-TCL3); classic mirrors maintained; extend/replace the
step-5 FLAG-only generic protection bridge with the final generic desired
store incl. params (D-CHM3); `recheck_channel_modes()` generic; lazy
re-parse on connect/isupport change; refactor JOIN-key + `*_prot` readers
to accessors (D-CHM4).

**Step 9 — ISUPPORT persistence.** Core global + `write_userfile` emit +
read capture gated to `bu == userlist` (D-ISU2..5); server.mod apply on
`userfile-loaded` via `isupport-default`.

**Step 10 — New Tcl introspection + docs.** Complete `chanmodeinfo`,
`isprefix`/`wasprefix`/`isprefixatleast`, and the already-started
`getchanmodes`/`chanmodelist` surfaces (D-TCL1); docs
(`tcl-commands.rst`, `bind mode` coverage, `opchars`/`NOHALFOPS_MODES`
deprecation, UPGRADING/NEWS).

## Test plan

The pytest harness (`tests/`, see `tests/README.md`) spawns the real
Eggdrop binary against a mock IRCd and asserts on internal state. Three
observation channels are used:

- **State via the bridge** — `tcl_bridge.eval_ok("…")` runs arbitrary Tcl
  inside the bot: `isop/ishalfop/isvoice/wasop/onchan/chanlist`,
  `getchanmode`, `chanbans/chanexempts/chaninvites`, `channel get <c>
  chanmode`, `isupport get <key>`, `set ::use-exempts`, and the **new**
  `getchanmodes/chanmodeinfo/isprefix/wasprefix/isprefixatleast/chanmodelist`
  commands. **Preferred** — robust, exact.
- **Wire via the mock** — `mock_ircd.send(raw)` injects 005/324/MODE/NAMES/
  numerics; `mock_ircd.expect_recv_match(regex)` / `drive_join_with_names`
  assert on what the bot *sends* (the only way to test `pushmode`/`flush`/
  enforcement output, e.g. that `MODE #c +j 3:5` actually goes out).
- **Log via stdout** — `eggdrop_proc.log_path` / `stdout_text()` for the
  `Learned mode type: …` debug lines and the parse-warning paths.

Existing coverage lives in `tests/tests/test_isupport_modes.py` (005
PREFIX/CHANMODES parsing, `.status all`, 324 with unknown modes, inbound
MODE with prefix/unknown/key mixing, use-exempts derivation).

### Working method (mandatory ordering)

1. **Characterization first.** Before writing any arbmodes3 code, land the
   "must-still-pass" tests below (step 0). They encode *current* behaviour.
   Run them green on `develop`'s behaviour, then keep them green through
   every step. A characterization test that goes red is a regression to
   investigate, not a test to edit — **except** the explicitly listed
   "intentionally changes" cases.
2. **Behaviour tests per step.** Each step lands its new-behaviour tests in
   the same commit; they are expected to be red before the step and green
   after.
3. **No `time.sleep`** — use `wait_for(...)` with a description, per README.

### A. Characterization tests — write before arbmodes3, must still pass after

These freeze the externally-observable contract of **every code path the
steps touch**. Eggdrop has essentially no existing coverage, so any touched
path without a characterization test is an unmitigated risk; the list below
was produced by auditing each step's touched functions (see the coverage
matrix). Files: `tests/tests/test_modes_characterization.py` plus themed
siblings (`test_modes_policy.py`, `test_modes_pushmode.py`,
`test_modes_enforcement.py`); the existing `test_isupport_modes.py` cases
already serve this role and must keep passing (exceptions in section D).

Test users are created via the bridge (`adduser`/`chattr`) so policy tests
can distinguish friend/op/deop/master victims. All "bot reacts" assertions
use `mock_ircd.expect_recv_match` / `drain_until`; all state assertions use
the bridge.

**Core state & formats**

- **A1 `getchanmode` format + bot status compatibility.** After `324 +ntkl
  secret 42`: flags contain `n t k l` (set membership, not order —
  D-CHM5), then `secret 42` in that exact arg order. Also
  `botisop`/`botishalfop`/`botisvoice` match the member-level
  `isop`/`ishalfop`/`isvoice` result for the bot across `@`/`%`/`+`/plain
  NAMES prefixes.
- **A2 `bind mode` args + `wasop` inside the bind.** Accumulate
  `"$mode|$victim|[wasop $victim $chan]"` from a `bind mode`; drive
  `MODE #c +o-o+v+b-l alice alice bob *!*@x`; assert sequence
  `+o|alice|0`, `-o|alice|1`, `+v|bob|…`, `+b|*!*@x|…`, `-l||…`
  (the historic `-l`→`""` arg quirk and WAS-state visibility in binds).
- **A3 member status from WHO — both flavours.** With WHOX (005 `WHOX`):
  `354` rows for `@op %hop +voice plain` ⇒ `isop/ishalfop/isvoice/wasop`
  booleans per member. Without WHOX: same via plain `352` flags. Away and
  bot metadata: flags `G` ⇒ `isaway`, BOT-flag char (005 `BOT=B`) ⇒
  `isircbot`; `H` ⇒ not away.
- **A4 NAMES path (userhost-in-names) stays status-blind.** With the cap
  REQ'd (`cap req userhost-in-names` via bridge + mock advertising it),
  NAMES `@alice!u@h` ⇒ `onchan alice` true, `getchanhost` set, and `isop
  alice` **false** (today's behaviour: the 353 path passes empty flags;
  D-PFX5 freezes this).
- **A5 netsplit WAS-state.** Member is `@op`; QUIT `:irc1.x irc2.y`
  (netsplit) ⇒ `onchansplit`; rejoin + server `MODE +o` ⇒ no punishment
  with `stopnethack-mode 2` (wasop satisfied); a *not-previously-opped*
  member server-opped under the same setting ⇒ bot sends `-o` (FAKEOP
  path).

**Outbound queue / pushmode**

- **A6 `pushmode` classic + dedup.** `pushmode #c +o alice; flushmode` ⇒
  `MODE #c +o alice`; duplicate `+o` while queued/SENT ⇒ no second line;
  `+b mask`, `-b mask` round out the queue types.
- **A7 `pushmode` key/limit slots.** `pushmode #c +k secret`, `+l 42`,
  later `-k secret` ⇒ correct `MODE` lines with args (today these ride
  dedicated `key/rmkey/limit` slots; after step 6 the same wire output must
  come from the generic queue).
- **A8 `prevent_mixing` e/I split.** `pushmode +b m1; pushmode +e m2;
  flushmode` ⇒ **two** MODE lines (default `prevent-mixing 1`).
- **A9 line splitting.** 005 `MODES=4` (post-clamp ≤ `MODES_PER_LINE_MAX`),
  queue 6 bans ⇒ two MODE lines of 4+2 with args aligned to letters.

**Enforcement & policy (the step-5 merge surface)**

- **A10 chanmode flag enforcement, live + on-join.** `chanmode +nt-i`:
  inbound `+i` ⇒ bot sends `-i`; inbound `-t` ⇒ `+t`; join with `324 +n`
  (missing `t`) ⇒ recheck pushes `+t`.
- **A11 key enforcement + JOIN key.** `chanmode +k wantkey`: bot's JOIN
  carries `wantkey`; inbound `-k old` ⇒ bot re-adds `+k wantkey`; stranger
  `+k other` ⇒ bot replaces with `wantkey` (got_key path). `chanmode -k`:
  inbound `+k foo` ⇒ bot sends `-k foo`.
- **A12 limit enforcement.** `chanmode +l 50`: inbound `-l` ⇒ `+l 50`;
  inbound `+l 10` ⇒ corrected to `+l 50` (mode_pls_prot CHANLIMIT path).
- **A13 +bitch.** Channel `+bitch`, op-by-user of a victim with no +o user
  flag ⇒ bot sends `-o victim`; victim with +o flag ⇒ no deop.
- **A14 protectops / protectfriends.** `-bitch +protectops`: user deops a
  victim whose handle has +o ⇒ bot re-ops; `+protectfriends` with +f victim
  ⇒ same.
- **A15 revenge representative.** `+revenge`, default `revenge-mode`:
  user deops a +f friend ⇒ bot punishes the deopper (assert the outbound
  `-o deopper`/kick per current default).
- **A16 bot-deop consequences.** Bot is deopped: `bind need` fires with
  `op`; queued SENT* state is cleared (a previously-suppressed `pushmode
  +o` now emits after re-op).
- **A17 autoop/autovoice.** `aop-delay 0:0`; `+autoop` channel + user with
  +o flag joins ⇒ bot sends `+o`; `+autovoice` + +v flag ⇒ `+v`
  (check_this_member/recheck path).
- **A18 enforcebans kick.** `+enforcebans`: inbound `+b` matching a member
  ⇒ bot kicks them (`kick_all`).
- **A19 ban on the bot bounces.** Inbound `+b` matching the bot's own
  n!u@h (no exempt) ⇒ bot sends `-b` (reversal path in got_ban).
- **A20 sticky/static ban re-add.** Sticky ban in userfile: another op's
  `-b` ⇒ bot re-adds `+b`. `-dynamicbans` with a userfile ban ⇒ pushed on
  join/recheck.
- **A21 nouser-list policy representative.** `-userexempts`: non-trusted
  user sets `+e mask` ⇒ bot removes it (same shape as nouserbans/-invites).
- **A22 desync/fake-mode kick.** MODE from a member the bot doesn't see as
  op/halfop (and `-nodesynch`) ⇒ bot kicks the sender and reverses
  (CHAN_FAKEMODE/DESYNC paths at the top of gotmode).
- **A23 bounce-modes.** `bounce-modes 1`: **server**-sourced `+i` (no nick
  prefix) ⇒ bot reverses `-i`; user-sourced `+i` ⇒ no bounce.

**Lists, persistence, misc**

- **A24 b/e/I list tracking.** `367/348/346` initial lists + live
  `+b/+e/+I`/`-…` ⇒ `chanbans/chanexempts/chaninvites` + `ischanban`
  reflect each change.
- **A25 chanmode round-trip + partyline.** `channel set #c chanmode "+mntk
  secret"` ⇒ `channel get` equivalent; partyline `.chanset #c chanmode
  +mnt` (separate code path in cmdschan.c) ⇒ same; persists across save +
  `.rehash`.
- **A26 partyline op commands.** `.op alice` ⇒ `MODE #c +o alice`;
  `.kickban badguy` ⇒ `+b` + `KICK` (cmdsirc.c HALFOP_CANTDOMODE→
  can_set_mode conversion sites).
- **A27 Undernet `key=*`.** `324 +k *` ⇒ after the bot is opped it re-asks
  `MODE #c` (CHAN_ASKEDMODES quirk, rewritten in step 7).
- **A28 bind-proc re-entrancy.** A `bind mode` proc that runs `channel
  remove #c` mid-burst ⇒ no crash, remaining modes of the burst are
  dropped cleanly (modebind_refresh contract).
- **A29 324 unknown-mode skipping** — existing tests
  (`test_got324_skips_unknown_mode_but_applies_key`, `+q` conflict); assert
  current skip-and-warn; migrate at step 7 (section C).
- **A30 use-exempts/use-invites derivation** (existing) — keep.
- **A31 `.status all` isupport line** (existing) — keep.

### B. New-behaviour tests — per step, red before / green after

- **Step 0** — `test_modes_per_line_clamp`: 005 `MODES=20`; queue ≥7 modes;
  `assert_alive`, emitted lines never exceed `MODES_PER_LINE_MAX` modes.
  Plus all of section A.
- **Step 2 (member prefixes)** — `multi-prefix` REQ'd (override `mock_ircd`
  caps; `cap enabled` contains it); WHOX `354` with `~&@%+` ⇒ `isop` is
  **literal-o** (false for `~`-only, true for `~@`); MODE `+q nick` on a
  prefix-q network consumes the nick and fires `bind mode`; `opchars` in
  conf ⇒ deprecation warning, no effect on recognition. Direct
  `isprefix`/`wasprefix` assertions are deferred to step 10, when those Tcl
  commands exist; arbitrary parameterized outbound `pushmode +q nick` waits
  for the step-6 queue rewrite.
- **Step 3 (capability)** — bot as `%`: `pushmode +v` emits, `+o`/`+h` do
  **not** (rank), `+b`/flags emit (non-prefix at-least-halfop); bot as `@`:
  `+o` emits (self-rank exception); on a quiet-LIST network bot-as-`%`
  `pushmode +q mask` is no longer capability-suppressed (legacy queueing
  still drops the LIST argument until the step-6 queue rewrite; the old
  `NOHALFOPS_MODES q` block is gone).
- **Steps 4–5 (generic dispatch + tracking)** — inbound `+S` (advertised
  FLAG) ⇒ in `getchanmodes`, `bind mode` fires `+S|`; `+j 3:5` ⇒
  `getchanmodes` has `j → 3:5`; quiet-LIST `+q *!*@x` ⇒ `chanmodelist #c q`
  has the mask and `getchanmode` has **no** `q` (D-CHM2 type-gating);
  flag-q network ⇒ inverse. `bounce-modes` bounces a server-set unknown
  flag (D10).
- **Step 6 (queue + pushmode)** — `pushmode #c +j 3:5` reaches the wire;
  `+b a; -b b; +b c; flushmode` ⇒ one line `+b-b+b a b c` (D-Q1);
  `pushmode +Z` (unknown) ⇒ Tcl **error**; `pushmode +j` (missing arg) ⇒
  error; A7's k/l wire output now from the generic queue; A8/A9 unchanged.
- **Step 7** — 324 with arbitrary flag modes appears in
  `getchanmode`/`getchanmodes`; A29 flips to tracking (section C); A27
  still passes.
- **Step 8 (channels.mod verbatim)** — `.chanset #c chanmode +zS` with
  letters unknown pre-connect: stored verbatim + warned, `channel get`
  returns it; post-connect enforcement of now-known flags (incl. param
  enforce `+j 3:5` per D-CHM3); LIST/PREFIX letters in chanmode ⇒ warn +
  ignore; reconnect with different CHANMODES re-derives.
- **Step 9 (ISUPPORT persistence)** — 005 with distinctive CHANMODES →
  `.save` ⇒ userfile has the `#isupport ` header (verbatim, multi-005
  concatenated in order); restart with that userfile, **no 005** ⇒
  `chanmodeinfo` reflects persisted values pre-connect; old-format userfile
  (no header) loads fine; `-m` flow unaffected (no userfile written before
  owner creation).
- **Step 10 (new Tcl commands)** — direct bridge tests of `chanmodeinfo`/
  `getchanmodes`/`isprefix`/`wasprefix`/`isprefixatleast`/`chanmodelist`
  arg handling and error cases.

### Coverage matrix — touched code → tests

| Touched code (step) | Tests |
|---|---|
| `irc_isupport` MODES clamp (0) | B0 |
| `process_prefix` rank, helpers, seeding (1) | B2, B10, A-suite green |
| `got352or4` flags parsing (2) | A3, A5, B2 |
| `got353` userhost-in-names (2, must not change) | A4 |
| member accessors / legacy flag mirror (2) | A2, A3, A5, B2 |
| `real_add_mode` SENT logic (2,6) | A6, A16 |
| `opchars` removal (2) | B2, D-rewrite of existing NAMES test |
| `can_set_mode` + HALFOP_CANTDOMODE sites in mode.c/chan.c (3) | B3, A19–A21 |
| cmdsirc.c command sites (3) | A26 |
| msgcmds.c sites (3) | accepted gap (below) |
| chanmode accessors, `set_key`, reset paths (4) | A1, A11, A27, B4 |
| `gotmode` dispatch: flag/key/limit/prefix/list cases (5) | A2, A10–A12, B4–B5 |
| op+halfop policy merge: bitch/stopnethack/protect*/revenge/need (5) | A13–A17, A5 |
| voice policy (5) | A17 |
| got_ban/exempt/invite policy (5, routed not rewritten) | A18–A21, A24 |
| desync/fake-kick preamble, bounce-modes, reversing (5) | A22, A23, B5 |
| `modebind_refresh` re-entrancy (5) | A28 |
| `flush_mode`/queue/`prevent_mixing`/limits (6) | A6–A9, B6 |
| `pushmode` validation (6) | B6 |
| `getchanmode`/`got324` rewrite (7) | A1, A27, A29→C, B7 |
| `set_mode_protect`/`get_mode_protect`/`recheck_channel_modes` (8) | A10–A12, A25, B8 |
| JOIN-key idiom sites (8) | A11 |
| `write_userfile`/`readuserfile` header (9) | B9 |
| new Tcl commands (10) | B10 |

### Accepted gaps — explicitly untested, with mitigation

- **share.mod** (`add_mode` from share, mask sync, userfile transfer
  `bu != userlist` gating): the harness has no multi-bot fixture. Mitigate:
  `add_mode`'s exported signature/behaviour for b/e/I is preserved
  verbatim; the step-9 read gate is a one-line condition reviewed in PR;
  flag share.mod smoke-testing as a manual pre-release item.
- **msgcmds.c** (/msg op|voice|invite…): same mechanical
  `HALFOP_CANTDOMODE→can_set_mode` conversion as cmdsirc.c, which A26
  covers; needs password setup to test. Mitigate: conversion is
  pattern-identical, covered by review.
- **flood-deop mass detection** (`detect_chan_flood FLOOD_DEOP`): timing-
  sensitive; A15 exercises the adjacent revenge path. Mitigate: that call
  site is moved, not modified.
- **twitch.mod** (D-OOS1): untouched by design; no tests.
- **Botnet propagation of chan settings**: out of scope, unchanged.

### C. Tests that migrate (skip→track) at step 7

`test_got324_skips_unknown_mode_but_applies_key` and
`test_got324_conflict_eggdrop_says_noargs_isupport_says_args` assert the
*current* "skip unknown / warn on conflict" behaviour. Once modes are
table-driven these become "track the previously-unknown mode." They are
**updated in the step-7 commit**, not before — and the PR description must
call out the behaviour change (a previously-skipped mode is now tracked).

### D. Tests that intentionally change — call out in the PR

- **`test_names_with_extended_prefix_grants_op_when_opchars_includes_it`**
  depends on `opchars`, which D-PFX5 removes. It is **rewritten** in step 2
  to assert the new contract: with WHOX/`multi-prefix`, a `~`-only owner is
  **not** `isop`, while `~@` is `isop` via literal `o`; generic owner state
  is asserted through `isprefix q`/`isprefixatleast o` when the step-10
  commands land. The old behaviour (owner counts as op because `opchars`
  includes `~`) is gone by design.
- **`getchanmode` flag ordering** (D-CHM5): any test asserting the exact
  flag-letter *order* must assert set membership instead. A1 is written this
  way from the start to avoid a churn.

### Coverage exit criteria for the PR

- All section-A characterization tests green on every commit (except C/D at
  their designated steps).
- Each step's section-B tests green in that step's commit.
- New Tcl introspection commands have direct bridge tests.
- A run under `make test` (coverage build) shows the new
  `gotmode`/accessor/queue paths exercised.

## Future to-dos (explicitly out of this branch)

- **Generic list-mode management command.** A DCC/Tcl command to *add*
  generic list-mode entries (quiet, …) with enforcement. **Sticky-or-not is
  unsolved** and entangled with the same questions as extbans — defer until
  designed.
- **Network-dependent list-query numerics — significant open problem.** The
  raw numeric returned when querying an arbitrary list mode (e.g. `MODE #c
  +q` → 728/729 on solanum/charybdis, other numerics elsewhere) is **not
  mapped by any ISUPPORT token**. There is no generic way to know which
  numeric a query will return or how to parse it, so generic list modes are
  delta-tracked only (no initial sync). Solving this is required before
  generic list modes can be complete on join; needs its own design.
- **Migrate b/e/I into the generic list store** one mode at a time, once the
  generic path is proven and the query problem above is solved.
- **share.mod/userfile records** for non-b/e/I list modes (D-OOS2).
- **Generalize policy** (bitch/stopnethack/protect/revenge) to owner/admin
  prefixes (the D-PFX1 asymmetry) — needs its own decision pass.
- **`prevent_mixing`/`compat`** generalization beyond the e/I split (D-Q3).
- **Hard-remove `opchars`** after the deprecation period (D-PFX5).
