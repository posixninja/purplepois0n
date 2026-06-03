#!/usr/bin/env bash
# Offline parser smoke checks (no device required).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"

if [[ ! -x "${BIN}" ]]; then
  echo "Building release..."
  make -C "${ROOT}" release
fi

run() {
  echo "--- $*"
  "${BIN}" "$@"
}

run --analyze-backup "${ROOT}/tests/fixtures/mbdb_minimal"
run --analyze-backup "${ROOT}/tests/fixtures/mbdb_v1_flat"
run --analyze-backup "${ROOT}/tests/fixtures/manifest_db_minimal"
run --analyze-backup "${ROOT}/tests/fixtures/manifest_db_keyed"
run --analyze-binary /bin/ls --arch arm64

if curl -sS --connect-timeout 1 http://127.0.0.1:3993/v1/_ping >/dev/null 2>&1; then
  echo "--- ipswd reachable; checking Backend: ipswd"
  out="$(run --analyze-binary /bin/ls --arch arm64 2>&1)"
  echo "${out}"
  echo "${out}" | grep -q 'Backend:       ipswd' || {
    echo "Expected Backend: ipswd when daemon is up" >&2
    exit 1
  }
else
  echo "--- ipswd not running (skipped; start with: make external-ipswd && ./external/ipsw/ipswd start)"
fi

echo "All fixture smoke checks passed."
