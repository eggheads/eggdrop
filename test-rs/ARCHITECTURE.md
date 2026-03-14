# test-rs Architecture

Rust-based integration test suite for Eggdrop. Tests run against the real
Eggdrop C code linked as a static library (`libeggdrop.a`), with network I/O
intercepted so no real IRC server is needed.

## Build pipeline

### 1. libeggdrop.a (Makefile)

`make libeggdrop.a` in the project root builds a static archive from the same
object files used for the normal eggdrop binary, including all statically-linked
modules. Two post-processing steps are applied with `objcopy`:

**main renaming** — `objcopy --redefine-sym main=eggdrop_main` renames the
entry point so the Rust test harness can call it as a library function from a
background thread. After archiving, the rename is reversed on `main.o` so that
normal `make` builds are unaffected.

**Symbol globalization** — Most internal variables in eggdrop are file-scope
`static`, making them invisible outside their translation unit. To allow tests
to inspect internal state (e.g. `nick_len` after ISUPPORT parsing), the
Makefile globalizes these symbols:

1. `nm` extracts all local symbols (lowercase `t`, `b`, `d` in nm output).
2. Colliders are identified and excluded:
   - Symbols that appear in multiple `.o` files (e.g. `global`, `gotmsg`) —
     globalizing these would cause duplicate-symbol link errors.
   - Local symbols whose name matches an existing global symbol (e.g.
     `encrypt_string` exists as both a static variable and a global function).
   - Section symbols (names starting with `.`).
3. The remaining ~1700 symbols are globalized via
   `objcopy --globalize-symbols=<file>`.

### 2. build.rs (Cargo build script)

The build script generates Rust FFI bindings from eggdrop's C headers using
`bindgen`, and handles the complexity of exposing globalized symbols.

**Link configuration** — Tells rustc to link against `libeggdrop.a` and its
dependencies (tcl, openssl, zlib, etc.).

**Macro collection** — Runs `clang -dM -E` on `wrapper.h` to collect all
preprocessor macro names. Eggdrop's module system exposes internal variables
through function-table macros like `#define nick_len (*(int *)(server_funcs[37]))`.
These macro names would be expanded by the preprocessor if used in `extern`
declarations, breaking compilation.

**Header symbol collection** — A first-pass bindgen run processes only
`wrapper.h` (the header chain) to discover which symbols already have typed
declarations in headers. These are excluded from `globals.h` to avoid
redeclaration conflicts (e.g. `global_bans` is declared in `chan.h`). This
pass also collects known type names (type aliases, structs, enums) for use
in the type visibility check.

**Type extraction via clang AST** — To get the real C types for globalized
symbols, `build.rs` maps `.o` files from the archive back to their `.c`
source files, then runs `clang -Xclang -ast-dump=json -fsyntax-only` on
each `.c` file in parallel threads. The JSON AST is parsed with `serde_json`
to find top-level `VarDecl` nodes with `storageClass: "static"`, extracting
each variable's `qualType` string (e.g. `"int"`, `"char [512]"`,
`"void (*)(int)"`).

A **type visibility check** (`type_is_visible()`) validates that all type
identifiers in the `qualType` string are either C keywords or types known
from the header-only bindgen pass. Module-internal types like `assoc_t` or
`isupport_t` (defined inside `.c` files, not headers) fail this check, and
the symbol falls back to `extern char name[];`.

**globals.h generation** — `nm -A` scans `libeggdrop.a` for global data
symbols (B and D types). For each symbol that is:
- a valid C identifier,
- not already declared in headers,
- not a duplicate,

an `extern` declaration is emitted. Macro-colliding symbols get a `#undef`
directive first. If the clang AST provided a type and the type is visible,
a properly typed declaration is emitted (e.g. `extern int nick_len;`,
`extern char botuserhost[121];`). Array types are formatted with brackets
after the identifier, and function pointers use the `(*name)` syntax.
Otherwise, a generic `extern char name[];` fallback is used.

This gives tests compile-time type safety — if the C type of a variable
changes, the Rust binding updates automatically on the next build.

**Final bindgen pass** — Processes `wrapper.h` + `globals.h` together, producing
`bindings.rs` with typed Rust bindings for both header-declared symbols and
globalized internal symbols.

### 3. wrapper.h

Includes eggdrop's header chain (`main.h`, `modules.h`, `modvals.h`) plus
module-specific headers (`server.h`). Compiled with `-DHAVE_CONFIG_H`,
`-DSTATIC`, and `-DMAKING_MODS` to match eggdrop's build environment.

## Runtime architecture

### Eggdrop lifecycle

`EggtestBuilder::spawn()` renders a config from `eggdrop.conf.j2` (a minijinja
template), installs DNS hook overrides, then calls `eggdrop_main()` in a
background thread. The config points at `fake.test-rs:6667` as the IRC server.

### DNS interception

Rust callbacks are registered via `add_hook(HOOK_DNS_IPBYHOST, ...)` and
`add_hook(HOOK_DNS_HOSTBYIP, ...)`. Forward lookups for `fake.test-rs` resolve
to `192.0.2.1` (RFC 5737 documentation range). This avoids real DNS queries.

### connect() interposition

A `#[no_mangle] extern "C" fn connect()` overrides libc's `connect()`. When
eggdrop connects to `192.0.2.1:6667`, the override creates a `socketpair()`,
splices one end onto eggdrop's fd via `dup2()`, and hands the other end to the
test as an `IRCd` handle. Non-matching connections pass through to the real
`connect()` (resolved via `dlsym(RTLD_NEXT, ...)`).

### Fake IRCd

The `IRCd` struct provides methods to send/receive IRC lines over the
socketpair:
- `negotiate()` — handles CAP LS, waits for NICK/USER, returns the registered nick
- `send_welcome()` — sends 001-005 burst with configurable ISUPPORT tokens
- `expect()` / `expect_prefix()` — wait for specific IRC lines with timeout
- `drain()` — consume pending lines

### Config templating

`eggdrop.conf.j2` is a minijinja template embedded at compile time via
`include_str!()`. The builder supports `with_set(key, value)` for injecting
settings, and `with_isupport(lines)` for customizing the 005 (ISUPPORT)
response sent during the welcome burst.

## File inventory

| File | Purpose |
|------|---------|
| `Cargo.toml` | Dependencies: libc, minijinja, tempfile; build-dep: bindgen, serde_json |
| `build.rs` | Link config, clang AST type extraction, nm analysis, bindgen |
| `wrapper.h` | Header chain for bindgen |
| `eggdrop.conf.j2` | Minijinja config template |
| `src/lib.rs` | Test harness: DNS hooks, connect interposition, IRCd, builder |
| `tests/connect.rs` | Verifies eggdrop registers (NICK/USER) on connect |
| `tests/ping.rs` | Verifies eggdrop responds to PING |
| `tests/isupport.rs` | Verifies ISUPPORT parsing (NICKLEN, WHOX, MODES, MAXLIST) |

## Alternatives considered

**`-Dstatic=extern` on C source files** — Feeding `.c` files (not just headers)
to bindgen with `#define static extern` would expose all file-scope variables
with their real types. However, this also transforms function-local statics
(e.g. `static char buf[512] = ...`) into `extern char buf[512] = ...`, which
is illegal C. Abandoned because eggdrop uses local statics extensively.

**`-Dstatic=extern` on headers only** — Works for variables declared in headers
but doesn't see `.c`-file statics like `nick_len`, which is the primary use
case.

**`objcopy --globalize-symbol='*'` (wildcard)** — Attempted instead of the
file-based approach. Fails because it also globalizes section symbols (`.text`,
`.data`, `.bss`), causing link-time collisions across object files.

**Macro type extraction from `#define` patterns** — Earlier iteration of the
type extraction approach. Parsed the expansion of function-table macros like
`#define nick_len (*(int *)(server_funcs[37]))` to extract the cast type.
Works for dereference-style macros but fails for plain casts like
`((char *)(funcs[N]))` and cannot extract types for variables not exposed
through the module macro system. Replaced by the clang AST approach which
covers all file-scope statics regardless of how they're exposed.

**Tree-sitter C parser** — Could parse `.c` files to extract `static` variable
declarations with full type information. Handles ~95% of eggdrop's patterns,
but fails on inline struct definitions (`static struct { ... } name;`), macro
qualifiers (`static int name STDVAR;`), and function pointers. The clang AST
approach handles all of these correctly since it uses clang's own parser.

**Regex-based C source parsing** — Simpler than tree-sitter but shares the same
failure modes with inline structs and macro qualifiers. Would also miss
multi-line declarations.

**Declaring all nm symbols as `extern void name()`** — First attempt at
globals.h generated function-style declarations for T (text) symbols alongside
data symbols. Failed because generic declarations conflicted with properly typed
declarations already in headers.
