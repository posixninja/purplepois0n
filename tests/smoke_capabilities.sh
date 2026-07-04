#!/usr/bin/env bash
# Offline smoke: --capabilities JSON (plugins, kpf, doctor).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${PURPLEPOIS0N_BIN:-$ROOT/build/bin/purplepois0n}"

if [[ ! -x "$BIN" ]]; then
  echo "skip: build purplepois0n first (make release)"
  exit 0
fi

OUT="$("$BIN" --capabilities 2>/dev/null | tail -1)" || {
  echo "FAIL: --capabilities failed"
  exit 1
}

echo "$OUT" | grep -q '"doctor":true' || {
  echo "FAIL: capabilities missing doctor"
  exit 1
}
echo "$OUT" | grep -q '"store":true' || {
  echo "FAIL: capabilities missing store"
  exit 1
}
echo "$OUT" | grep -q '"kpf"' || {
  echo "FAIL: capabilities missing kpf block"
  exit 1
}

echo "smoke-capabilities: OK"
