#!/usr/bin/env bash
# Offline smoke for medicine post-install cure planning.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"

if [[ ! -x "$BIN" ]]; then
  echo "Building purplepois0n..."
  make -C "$ROOT" release
fi

HELP="$("$BIN" --help 2>&1 || true)"
echo "$HELP" | grep -q -- "--medicine-probe" || {
  echo "FAIL: --medicine-probe missing from help"
  exit 1
}
echo "$HELP" | grep -q -- "--medicine-apply" || {
  echo "FAIL: --medicine-apply missing from help"
  exit 1
}
echo "$HELP" | grep -q -- "--medicine-cures" || {
  echo "FAIL: --medicine-cures missing from help"
  exit 1
}

echo "OK: medicine CLI wired"

# Probe-only without device: should log plan and exit 0 (no USB required for empty udid path)
if "$BIN" --medicine-probe --medicine-cures all 2>&1 | grep -q "\[Medicine\]"; then
  echo "OK: medicine probe logs cure plan"
else
  echo "FAIL: medicine probe produced no [Medicine] output"
  exit 1
fi

echo "smoke_medicine: passed"
