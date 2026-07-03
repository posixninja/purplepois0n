#!/usr/bin/env bash
# smoke_dtree_mmio.sh — DeviceTree register inventory (offline fixture)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"
FIXTURE="${ROOT}/tests/fixtures/devicetree_mmio_sample.json"
OUT="${TMPDIR:-/tmp}/pp-dtree-mmio-smoke-$$.json"

cleanup() {
  rm -f "$OUT"
}
trap cleanup EXIT

make -C "$ROOT" release >/dev/null

PURPLEPOIS0N_MMIO_CATALOG="$FIXTURE" "$BIN" --probe-primitive devicetree-mmio

"$BIN" --dtree-registers "$FIXTURE" --dtree-registers-out "$OUT" --dtree-registers-verbose

test -f "$OUT"
grep -q '"kind": "pmap-io-ranges"' "$OUT"
grep -q '"kind": "ps-regs"' "$OUT"
grep -q '"kind": "pmgr-device"' "$OUT"
grep -q '"kind": "reg-private"' "$OUT"
grep -q '"kind": "interrupt"' "$OUT"
grep -q '"tag": "agx-gpu"' "$OUT"

"$BIN" --dtree-mmio "$FIXTURE" --dtree-mmio-out "${OUT}.mmio"
grep -q '"registers":' "${OUT}.mmio"

echo "smoke_dtree_mmio: OK"
