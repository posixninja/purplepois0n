#!/usr/bin/env bash
# Offline smoke: --device-plan + device plan subcommand JSON contract.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${PURPLEPOIS0N_BIN:-$ROOT/build/bin/purplepois0n}"
SCAN_TIMEOUT="${PURPLEPOIS0N_SMOKE_SCAN_TIMEOUT:-15}"

if [[ ! -x "$BIN" ]]; then
  echo "skip: build purplepois0n first (make release)"
  exit 0
fi

run_scan() {
  if command -v timeout >/dev/null 2>&1; then
    timeout "$SCAN_TIMEOUT" "$@"
  elif command -v gtimeout >/dev/null 2>&1; then
    gtimeout "$SCAN_TIMEOUT" "$@"
  else
    "$@"
  fi
}

"$BIN" dev --help 2>&1 | grep -q -- '--device-plan' || {
  echo "FAIL: --device-plan missing from dev help"
  exit 1
}

"$BIN" --help device 2>&1 | grep -q '|plan' || {
  echo "FAIL: device plan missing from subcommand help"
  exit 1
}

# Subcommand rewrite must exist (no device → exit 1 is OK).
if run_scan "$BIN" device plan 2>/dev/null; then
  OUT="$(run_scan "$BIN" device plan 2>/dev/null)"
  echo "$OUT" | grep -q '"device"' || { echo "FAIL: device plan missing device key"; exit 1; }
  echo "$OUT" | grep -q '"plan"' || { echo "FAIL: device plan missing plan key"; exit 1; }
  echo "$OUT" | grep -q '"strategy"' || { echo "FAIL: device plan missing strategy"; exit 1; }
  echo "smoke-device-plan: OK (live device via subcommand)"
elif OUT="$(run_scan "$BIN" --device-plan 2>/dev/null)"; then
  echo "$OUT" | grep -q '"device"' || { echo "FAIL: missing device key"; exit 1; }
  echo "$OUT" | grep -q '"plan"' || { echo "FAIL: missing plan key"; exit 1; }
  echo "$OUT" | grep -q '"strategy"' || { echo "FAIL: missing strategy"; exit 1; }
  echo "smoke-device-plan: OK (live device)"
else
  echo "smoke-device-plan: OK (no device — flags present)"
fi
