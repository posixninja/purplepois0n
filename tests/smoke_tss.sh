#!/usr/bin/env bash
# Host-side TSS / futurerestore smoke (no device required).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"

if [[ ! -x "${BIN}" ]]; then
  echo "Building release..."
  make -C "${ROOT}" release LIBTATSU=0
fi

echo "--- TSS probe (no IPSW)"
tss_out="$("${BIN}" --tss-check 2>&1 || true)"
echo "${tss_out}" | grep -F '[TSS]' >/dev/null || {
  echo "Expected TSS probe log" >&2
  exit 1
}

FAKE_IPSW="/tmp/pp-fake.ipsw"
FAKE_TICKET="/tmp/pp-fake.shsh2"
touch "${FAKE_IPSW}" "${FAKE_TICKET}"

echo "--- SavedApTicket planning without futurerestore (expect error)"
saved_out="$("${BIN}" --tss-check --ipsw "${FAKE_IPSW}" --apticket "${FAKE_TICKET}" 2>&1 || true)"
echo "${saved_out}" | grep -F 'futurerestore required' >/dev/null || {
  echo "Expected futurerestore required message" >&2
  exit 1
}
echo "OK: missing futurerestore detected"

rm -f "${FAKE_IPSW}" "${FAKE_TICKET}"

echo "--- Pongo boot probe report"
chmod +x "${ROOT}/tests/assert_chain_report.sh"
"${ROOT}/tests/assert_chain_report.sh" /tmp/pp-pongo-probe.json \
  --pongo-boot --pongo-kpf /tmp/x --pongo-ramdisk /tmp/x.dmg

echo "TSS host smoke checks passed."
