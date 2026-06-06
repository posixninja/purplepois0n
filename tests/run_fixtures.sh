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
run --analyze-crash "${ROOT}/tests/fixtures/crash_minimal.ips"
run --analyze-binary /bin/ls --arch arm64

ENT="${ROOT}/tests/fixtures/ents/jailbreak-helper.plist"
SIGNED="/tmp/pp-ls-signed"
rm -f "${SIGNED}"
if [[ -x "${ROOT}/external/ipsw/ipsw" ]]; then
  run --sign-macho /bin/ls --ad-hoc --sign-id com.test.ls --ent "${ENT}" --output "${SIGNED}"
  test -f "${SIGNED}"
  rm -f "${SIGNED}"
else
  echo "--- ipsw not built (skipped --sign-macho; run: make external-ipsw)"
fi

RAMDISK_DMG="/tmp/pp-test-ramdisk.dmg"
rm -f "${RAMDISK_DMG}"
run --build-ramdisk "${RAMDISK_DMG}" --ramdisk-overlay "${ROOT}/tests/fixtures/ramdisk_overlay"
test -f "${RAMDISK_DMG}"
if [[ -x "${ROOT}/external/ipsw/ipsw" ]]; then
  echo "--- ipsw disk hfs ${RAMDISK_DMG}"
  "${ROOT}/external/ipsw/ipsw" disk hfs "${RAMDISK_DMG}" | grep -q hello.txt || {
    echo "Expected hello.txt in ramdisk listing" >&2
    exit 1
  }
else
  echo "--- ipsw not built (skipped disk hfs verify)"
fi
rm -f "${RAMDISK_DMG}"

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

if [[ -f "${ROOT}/tests/assert_chain_report.sh" ]]; then
  echo "--- chain report: pongo-boot probe"
  chmod +x "${ROOT}/tests/assert_chain_report.sh"
  "${ROOT}/tests/assert_chain_report.sh" /tmp/pp-fixture-pongo.json \
    --pongo-boot --pongo-kpf /tmp/x --pongo-ramdisk /tmp/x.dmg
fi

echo "All fixture smoke checks passed."
