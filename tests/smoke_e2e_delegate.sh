#!/usr/bin/env bash
# smoke_e2e_delegate.sh — post-jailbreak E2E (device must be jailbroken OR palera1n installed)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"
UDID="${PURPLEPOIS0N_DEVICE_UDID:-}"

echo "--- build plugins"
make -C "${ROOT}" plugins LIBTATSU=0 >/dev/null

if [[ -z "$UDID" ]]; then
  echo "smoke_e2e_delegate: SKIP (set PURPLEPOIS0N_DEVICE_UDID for hardware test)"
  exit 0
fi

echo "--- seed store"
"${ROOT}/legacy/scripts/seed-store.sh" >/dev/null

export PURPLEPOIS0N_NORMAL_SSH=1
export PURPLEPOIS0N_RAMDISK_AUTO_IPROXY=1

echo "--- external jailbreak probe (already jailbroken path)"
if ! "${BIN}" --external-jailbreak --already-jailbroken --normal-ssh -d "$UDID"; then
  echo "smoke_e2e_delegate: /var/jb probe failed — jailbreak device first (palera1n or Dopamine)" >&2
  exit 1
fi

echo "--- store sync + install smoke package"
"${BIN}" store sync -d "$UDID" --store-root "${ROOT}/store"
"${BIN}" store install purplepois0n-smoke -d "$UDID" --store-root "${ROOT}/store"

echo "smoke_e2e_delegate passed"
