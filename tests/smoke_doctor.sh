#!/usr/bin/env bash
# Offline smoke: --doctor-run emits JSON steps even without a device.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${PURPLEPOIS0N_BIN:-$ROOT/build/bin/purplepois0n}"

if [[ ! -x "$BIN" ]]; then
  echo "skip: build purplepois0n first (make release)"
  exit 0
fi

OUT="$("$BIN" --doctor-run 2>&1 || true)"
echo "$OUT" | grep -q '"type":"step"' || {
  echo "FAIL: expected step JSON from --doctor-run"
  echo "$OUT"
  exit 1
}
echo "$OUT" | grep -q '"type":"complete"' || {
  echo "FAIL: expected complete JSON from --doctor-run"
  echo "$OUT"
  exit 1
}
echo "smoke-doctor: OK"
