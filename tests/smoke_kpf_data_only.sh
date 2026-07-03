#!/usr/bin/env bash
# kpf-purple v0.5 default: data-only patches (cstring + __DATA_CONST).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
KC="${KPF_PURPLE_KERNELCACHE:-${1:-}}"
if [[ -z "${KC}" && -d /tmp/pp-kpf-fw ]]; then
  KC="$(find /tmp/pp-kpf-fw -name 'kernelcache.release.*' -type f 2>/dev/null | head -1 || true)"
fi
BUILD_SCRIPT="${ROOT}/legacy/scripts/kpf-build.sh"
KPF_TEST="${ROOT}/legacy/kpf-purple/build/kpf-test-purple.macos"

chmod +x "${BUILD_SCRIPT}" 2>/dev/null || true
"${BUILD_SCRIPT}" test

if [[ -z "${KC}" || ! -f "${KC}" ]]; then
  echo "KPF_PURPLE_KERNELCACHE not set — build-only data-only smoke passed"
  exit 0
fi

LOG="$(mktemp)"
"${KPF_TEST}" "${KC}" >"${LOG}" 2>&1

grep -q "data-only mode" "${LOG}" || { echo "missing data-only banner" >&2; tail -20 "${LOG}"; exit 1; }
grep -q "mode=data-only" "${LOG}" || { echo "missing data-only summary" >&2; tail -20 "${LOG}"; exit 1; }
grep -q "snapshot=1" "${LOG}" || echo "note: snapshot string not present on this kernel"

if grep -q "KPF: Found AMFI" "${LOG}"; then
  echo "FAIL: text AMFI patch ran in data-only mode" >&2
  exit 1
fi

grep "patch summary" "${LOG}"
echo "OK: kpf data-only smoke passed"
rm -f "${LOG}"
