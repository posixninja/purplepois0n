#!/usr/bin/env bash
# Run purple kpf-test and upstream checkra1n kpf-test on the same kernelcache.
#
# Usage: kpf-compare-upstream.sh KERNELCACHE
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
KC="${1:-${KPF_PURPLE_KERNELCACHE:-}}"
PONGO="${PONGO_ROOT:-${ROOT}/legacy/modern-era/PongoOS}"
PURPLE_TEST="${ROOT}/legacy/kpf-purple/build/kpf-test-purple.macos"
UPSTREAM_DIR="${PONGO}/checkra1n/kpf-test"

if [[ -z "${KC}" || ! -f "${KC}" ]]; then
  echo "Usage: $0 KERNELCACHE" >&2
  exit 1
fi

"${ROOT}/legacy/scripts/kpf-build.sh" test

if [[ ! -d "${PONGO}/.git" ]]; then
  "${ROOT}/legacy/scripts/kpf-build.sh" sync
fi

if [[ ! -x "${UPSTREAM_DIR}/kpf-test.macos" ]]; then
  echo "--- building upstream kpf-test.macos"
  make -C "${UPSTREAM_DIR}" kpf-test.macos KPF_CFLAGS="-Wno-error=unused-but-set-variable"
fi

run_one() {
  local label="$1"
  local bin="$2"
  echo ""
  echo "======== ${label} ========"
  "${bin}" "${KC}" 2>&1 | rg '^(KPF:|panic:|# purplepois0n|# checkra1n|patch summary)' || true
}

run_one "purplepois0n kpf-purple" "${PURPLE_TEST}"
run_one "checkra1n upstream kpf-test" "${UPSTREAM_DIR}/kpf-test.macos"
