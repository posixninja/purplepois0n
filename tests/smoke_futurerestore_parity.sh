#!/usr/bin/env bash
# Offline futurerestore argv parity smoke (no device, no real restore).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"

if [[ ! -x "${BIN}" ]]; then
  echo "Building release..."
  make -C "${ROOT}" release LIBTATSU=0
fi

FAKE_FR="${ROOT}/build/tmp-fake-futurerestore.sh"
mkdir -p "${ROOT}/build"
cat >"${FAKE_FR}" <<'EOF'
#!/usr/bin/env bash
echo "FAKE_FR $*"
EOF
chmod +x "${FAKE_FR}"

FAKE_IR="${ROOT}/build/tmp-fake-idevicerestore.sh"
cat >"${FAKE_IR}" <<'EOF'
#!/usr/bin/env bash
echo "FAKE_IR $*"
EOF
chmod +x "${FAKE_IR}"

FAKE_IPSW="/tmp/pp-fr-parity.ipsw"
T1="/tmp/pp-fr-t1.shsh2"
T2="/tmp/pp-fr-t2.shsh2"
touch "${FAKE_IPSW}" "${T1}" "${T2}"

export PURPLEPOIS0N_FUTURERESTORE="${FAKE_FR}"
export PURPLEPOIS0N_IDEVICERESTORE="${FAKE_IR}"

probe_fr() {
  "${BIN}" --tss-check --ipsw "${FAKE_IPSW}" "$@" 2>&1 || true
}

assert_contains() {
  local haystack="$1"
  local needle="$2"
  if ! grep -Fq -- "${needle}" <<<"${haystack}"; then
    echo "Expected output to contain: ${needle}" >&2
    echo "--- output ---" >&2
    echo "${haystack}" >&2
    exit 1
  fi
}

echo "--- futurerestore full flag argv"
out="$(probe_fr --apticket "${T1}" --apticket "${T2}" --latest-sep --latest-baseband \
  --update --wait --use-pwndfu --just-boot --just-boot-args '-v' --debug-restore \
  --sep /tmp/sep.im4p --sep-manifest /tmp/sep.plist \
  --baseband /tmp/bb.im4p --baseband-manifest /tmp/bb.plist)"
assert_contains "${out}" "futurerestore command (probe only)"
assert_contains "${out}" "--update"
assert_contains "${out}" "-w"
assert_contains "${out}" "--use-pwndfu"
assert_contains "${out}" "--just-boot"
assert_contains "${out}" "-v"
assert_contains "${out}" "--debug"
assert_contains "${out}" "--latest-sep"
assert_contains "${out}" "--latest-baseband"
assert_contains "${out}" "-t ${T1}"
assert_contains "${out}" "-t ${T2}"

echo "--- futurerestore --exit-recovery"
out="$(probe_fr --apticket "${T1}" --exit-recovery)"
assert_contains "${out}" "--exit-recovery"

echo "--- idevicerestore stock argv"
out="$("${BIN}" --tss-check --ipsw "${FAKE_IPSW}" --update --debug-restore 2>&1 || true)"
assert_contains "${out}" "idevicerestore command (probe only)"
assert_contains "${out}" "-u"
assert_contains "${out}" "-d"

echo "--- no-baseband Wi-Fi path"
out="$(probe_fr --apticket "${T1}" --no-baseband --latest-sep)"
assert_contains "${out}" "--no-baseband"

rm -f "${FAKE_IPSW}" "${T1}" "${T2}"
echo "futurerestore parity smoke passed."
