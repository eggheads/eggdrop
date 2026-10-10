#!/usr/bin/env bash
#
# Generate a line-coverage report for Eggdrop from a clean tree.
#
# Run from the repository root:  ./coverage.sh
#
# Why this is more involved than "configure --enable-coverage && lcov":
#
#   1. Every test spawns a fresh eggdrop. Without redirection they all merge
#      into the same .gcda files, and concurrent merges corrupt them -- you
#      get negative counters and impossible >100% rates. GCOV_PREFIX gives
#      each test its own output tree so no merge ever happens.
#   2. GCOV_PREFIX moves the .gcda but not the .gcno notes, which stay in the
#      build tree. lcov needs them side by side, so we copy the notes into
#      each per-test tree before capturing.
#   3. Branch coverage is unreliable here regardless -- GCC's branch counters
#      lose updates in eggdrop's threaded DNS paths. Line coverage only.
#
# Prerequisites: build-essential autoconf tcl-dev libssl-dev lcov rsync uv
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASETEMP="${BASETEMP:-/tmp/eggcov}"
OUTDIR="${OUTDIR:-$REPO_ROOT/coverage-html}"
INFO="$REPO_ROOT/coverage.info"

cd "$REPO_ROOT"

say() { printf '\n\033[1m==> %s\033[0m\n' "$*"; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------- 0. checks
say "Checking prerequisites"
for t in gcc make lcov genhtml gcov rsync uv; do
  command -v "$t" >/dev/null || die "$t not found"
done
grep -q "GCOV_PREFIX" tests/conftest.py \
  || die "tests/conftest.py has no GCOV_PREFIX -- see the patch in the notes below"

# ---------------------------------------------------- 1. clean instrumented build
say "Clean build with coverage instrumentation"
make distclean >/dev/null 2>&1 || true
find . \( -name '*.gcda' -o -name '*.gcno' \) -delete
rm -rf "$BASETEMP" "$INFO" "$OUTDIR"

./configure --enable-coverage
make config
make debug

grep -q -- "--coverage" src/Makefile     || die "core CFLAGS missing --coverage"
grep -q -- "--coverage" src/mod/Makefile || die "module CFLAGS missing --coverage"
[ -f src/main.gcno ]                     || die "no .gcno produced; build did not recompile"
say "Build OK -- $(find . -name '*.gcno' | wc -l) notes files"

# ------------------------------------------------------------- 2. run the suite
say "Running test suite (writing coverage to $BASETEMP)"
( cd tests && uv sync --quiet && \
  uv run pytest --basetemp="$BASETEMP" -v\
      --html=report.html --self-contained-html "$@" || true )

GCDA_COUNT=$(find "$BASETEMP" -name '*.gcda' | wc -l)
[ "$GCDA_COUNT" -gt 0 ] || die "no .gcda under $BASETEMP -- is GCOV_PREFIX reaching the bot?"
find . -name '*.gcda' | grep -q . \
  && echo "WARNING: .gcda leaked into the source tree; some process ignored GCOV_PREFIX"
say "Collected $GCDA_COUNT data files across $(find "$BASETEMP" -maxdepth 1 -type d | wc -l) test dirs"

# ------------------------------- 3. put the notes next to the data lcov expects
say "Merging per-test coverage data"
MERGED=/tmp/eggcov-merged
rm -rf "$MERGED"
first=1
for d in "$BASETEMP"/*/gcov; do
  [ -d "$d/src" ] || continue
  if [ $first = 1 ]; then cp -a "$d" "$MERGED"; first=0
  else
    gcov-tool merge "$MERGED" "$d" -o "$MERGED.new" >/dev/null 2>&1 \
      && rm -rf "$MERGED" && mv "$MERGED.new" "$MERGED"
  fi
done
rsync -a --include='*/' --include='*.gcno' --exclude='*' src/ "$MERGED/src/"

# ------------------------------------------------------------------ 4. capture
say "Capturing coverage"
lcov --capture --directory "$MERGED" \
     --output-file "$INFO" \
     --ignore-errors source,unused,empty,negative

lcov --remove "$INFO" '/usr/*' '*/tcl*/generic/*' \
     --output-file "$INFO" \
     --ignore-errors unused,empty

# ------------------------------------------------------------------- 5. report
say "Summary"
lcov --list "$INFO"

if lcov --list "$INFO" | awk '/%/ {gsub(/%/,"",$2); if ($2+0 > 100) exit 1}'; then
  echo
  echo "Sanity check passed: no coverage rate exceeds 100%."
else
  echo
  echo "WARNING: a rate above 100% means the counters are still corrupt."
  echo "         Do not trust these numbers."
fi

genhtml "$INFO" --output-directory "$OUTDIR" --ignore-errors source

# ------------------------------------------------- 6. combined landing page
# genhtml owns $OUTDIR/index.html, so coverage lives under coverage/ and a
# generated index.html at the top links to both reports.
say "Building combined report"
REPORT="$REPO_ROOT/report"
rm -rf "$REPORT"
mkdir -p "$REPORT"
mv "$OUTDIR" "$REPORT/coverage"

if [ -f "$REPO_ROOT/tests/report.html" ]; then
  mv "$REPO_ROOT/tests/report.html" "$REPORT/tests.html"
  TESTS_LINK='<li><a href="tests.html">Test results</a> &mdash; pass/fail per test, with output</li>'
else
  TESTS_LINK='<li><em>No test report found.</em> Re-run with <code>--html=report.html --self-contained-html</code> to include one.</li>'
fi

TOTAL_LINE=$(lcov --list "$INFO" 2>/dev/null | tail -2 | head -1)

cat > "$REPORT/index.html" <<HTML
<!doctype html>
<meta charset="utf-8">
<title>Eggdrop test &amp; coverage report</title>
<style>
  body { font: 16px/1.6 system-ui, sans-serif; max-width: 46rem;
         margin: 3rem auto; padding: 0 1rem; color: #222; }
  h1 { font-size: 1.5rem; margin-bottom: .25rem; }
  .meta { color: #666; font-size: .875rem; margin-bottom: 2rem; }
  ul { padding-left: 1.2rem; }
  li { margin: .5rem 0; }
  pre { background: #f5f5f5; padding: .75rem; overflow-x: auto;
        font-size: .8125rem; }
  .warn { background: #fff8e1; border-left: 3px solid #f0ad4e;
          padding: .75rem 1rem; font-size: .875rem; }
</style>
<h1>Eggdrop test &amp; coverage report</h1>
<p class="meta">Generated $(date -u '+%Y-%m-%d %H:%M UTC') on $(hostname)</p>
<ul>
  $TESTS_LINK
  <li><a href="coverage/index.html">Coverage</a> &mdash; line coverage per file</li>
</ul>
<h2>Summary</h2>
<pre>$TOTAL_LINE</pre>
<p class="warn">
  Line coverage only. Branch counters are unreliable on this build &mdash;
  GCC's branch instrumentation loses updates in Eggdrop's threaded DNS
  paths, so <code>--branch-coverage</code> is deliberately not used.
</p>
HTML

say "Combined report: $REPORT/index.html"
