#!/usr/bin/env bash
# Host kernelcache patchfind + apply smoke (no device).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"

if [[ ! -x "${BIN}" ]]; then
  echo "Building release..."
  make -C "${ROOT}" release LIBTATSU=0
fi

INPUT="/tmp/pp-host-patch-input.bin"
OUTPUT="/tmp/pp-host-patch-out.bin"
PROFILE="${ROOT}/tests/fixtures/patch_profile_minimal.json"
cp /bin/ls "${INPUT}"

echo "--- host patchfind"
"${BIN}" --kernelcache "${INPUT}"

if make -C "${ROOT}" -q plugins 2>/dev/null || [[ -x "${ROOT}/build/bin/purplepois0n" ]]; then
  if make -C "${ROOT}" plugins >/dev/null 2>&1; then
    echo "--- host patch apply (plugins build)"
    rm -f "${OUTPUT}"
    "${BIN}" --kernelcache "${INPUT}" --patch-profile "${PROFILE}" --patch-out "${OUTPUT}"
    test -f "${OUTPUT}"
    python3 - <<PY
import sys
with open("${INPUT}", "rb") as a, open("${OUTPUT}", "rb") as b:
    da, db = a.read(4), b.read(4)
if da == db:
    sys.exit("expected first 4 bytes to change after patch")
print("OK: patch applied")
PY
    rm -f "${INPUT}" "${OUTPUT}"
  else
    echo "--- plugins build skipped (optional)"
    rm -f "${INPUT}"
  fi
else
  rm -f "${INPUT}"
fi

echo "Host patch smoke passed."
