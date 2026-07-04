#!/usr/bin/env bash
# smoke_store_device.sh — optional real-device store sync (skipped without UDID)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"
UDID="${PURPLEPOIS0N_DEVICE_UDID:-}"

if [[ -z "$UDID" ]]; then
  echo "smoke_store_device: SKIP (set PURPLEPOIS0N_DEVICE_UDID for hardware test)"
  exit 0
fi

make -C "$ROOT" release >/dev/null
legacy/scripts/seed-store.sh >/dev/null

echo "smoke_store_device: syncing to $UDID"
PURPLEPOIS0N_NORMAL_SSH=1 "$BIN" store sync -d "$UDID" --store-root "$ROOT/store"

echo "smoke_store_device: installing purplepois0n-smoke"
PURPLEPOIS0N_NORMAL_SSH=1 "$BIN" store install purplepois0n-smoke -d "$UDID" --store-root "$ROOT/store"

echo "smoke_store_device: OK"
