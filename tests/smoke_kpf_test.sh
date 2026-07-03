#!/usr/bin/env bash
# Offline kpf-purple smoke: data-only by default; set KPF_ALLOW_TEXT=1 for __TEXT_EXEC regression.
#
# Env:
#   KPF_PURPLE_KERNELCACHE   Path to kernelcache (required for patch match test)
#   PONGO_ROOT               Override PongoOS mirror path
#   PURPLEPOIS0N_IPSW        ipsw binary for kernel metadata
#
# Device smoke (optional, A5–A11 + Pongo shell):
#   checkra1n -cp
#   purplepois0n --pongo-probe
#   purplepois0n --pongo-execute --pongo-kpf legacy/kpf-purple/build/purplepois0n-kpf-pongo \
#     --pongo-ramdisk /path/to/raw.dmg
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
KC="${KPF_PURPLE_KERNELCACHE:-${1:-}}"
if [[ -z "${KC}" && -d /tmp/pp-kpf-fw ]]; then
  KC="$(find /tmp/pp-kpf-fw -name 'kernelcache.release.*' -type f 2>/dev/null | head -1 || true)"
fi
BUILD_SCRIPT="${ROOT}/legacy/scripts/kpf-build.sh"
KPF_TEST="${ROOT}/legacy/kpf-purple/build/kpf-test-purple.macos"
INFO_SCRIPT="${ROOT}/legacy/scripts/kpf-kernel-info.sh"

chmod +x "${BUILD_SCRIPT}" "${INFO_SCRIPT}" 2>/dev/null || true

echo "--- sync + build kpf-purple"
"${BUILD_SCRIPT}" all

if [[ ! -x "${KPF_TEST}" ]]; then
  echo "kpf-test-purple.macos missing after build" >&2
  exit 1
fi

if [[ -z "${KC}" ]]; then
  echo "KPF_PURPLE_KERNELCACHE not set — build-only smoke passed"
  echo "Set KPF_PURPLE_KERNELCACHE=/path/to/kernelcache for AMFI patch validation"
  exit 0
fi

if [[ ! -f "${KC}" ]]; then
  echo "kernelcache not found: ${KC}" >&2
  exit 1
fi

echo "--- ipsw kernel metadata"
"${INFO_SCRIPT}" "${KC}" || true

echo "--- kpf-test-purple on ${KC} (KPF_ALLOW_TEXT=${KPF_ALLOW_TEXT:-0})"
LOG="$(mktemp)"
set +e
env KPF_ALLOW_TEXT="${KPF_ALLOW_TEXT:-0}" "${KPF_TEST}" "${KC}" 2>&1 | tee "${LOG}"
RC=${PIPESTATUS[0]}
set -e

if [[ "${RC}" -ne 0 ]]; then
  echo "kpf-test-purple failed (exit ${RC})" >&2
  rm -f "${LOG}"
  exit "${RC}"
fi

if [[ "${KPF_ALLOW_TEXT:-0}" == "1" ]]; then
  if ! grep -q "KPF: Found AMFI" "${LOG}"; then
    echo "expected KPF: Found AMFI in text mode" >&2
    rm -f "${LOG}"
    exit 1
  fi
  echo "OK: text-mode AMFI patch matched"
else
  if ! grep -q "mode=data-only" "${LOG}"; then
    echo "expected data-only summary (set KPF_ALLOW_TEXT=1 for text regression)" >&2
    rm -f "${LOG}"
    exit 1
  fi
  if grep -q "KPF: Found AMFI" "${LOG}"; then
    echo "unexpected text AMFI patch in data-only mode" >&2
    rm -f "${LOG}"
    exit 1
  fi
  echo "OK: data-only mode (no __TEXT_EXEC patches)"
fi

for needle in "Patched dyld check" "AMFI hashtype check" "AMFI execve hook" \
              "Found mac_mount" "Found dounmount" "Disabled snapshot temporarily" \
              "Found task_conversion_eval" "Found vm_map_protect" "Found vm_fault_enter" \
              "Found convert_port_to_map_with_flavor"; do
  if grep -q "${needle}" "${LOG}"; then
    echo "OK: ${needle}"
  else
    echo "note: ${needle} not matched (optional on this kernel)"
  fi
done

rm -f "${LOG}"

MODULE="${ROOT}/legacy/kpf-purple/build/purplepois0n-kpf-pongo"
if [[ -f "${MODULE}" ]]; then
  echo "OK: Pongo module at ${MODULE}"
fi

echo "OK: kpf-purple smoke passed."
