#!/usr/bin/env bash
# Dump kernel metadata via ipsw for KPF patch development.
#
# Usage:
#   kpf-kernel-info.sh KERNELCACHE [IPSW]
#
# Env:
#   PURPLEPOIS0N_IPSW  ipsw binary (else external/ipsw build or PATH)
#
# Outputs JSON-friendly logs: Darwin version, AMFI kext presence, Mach-O summary.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
KC="${1:-}"
IPSW_PATH="${2:-}"

if [[ -z "${KC}" ]]; then
  echo "Usage: $0 KERNELCACHE [IPSW]" >&2
  exit 1
fi
if [[ ! -f "${KC}" ]]; then
  echo "kernelcache not found: ${KC}" >&2
  exit 1
fi

find_ipsw() {
  if [[ -n "${PURPLEPOIS0N_IPSW:-}" && -x "${PURPLEPOIS0N_IPSW}" ]]; then
    echo "${PURPLEPOIS0N_IPSW}"
    return
  fi
  if [[ -x "${ROOT}/external/ipsw/ipsw" ]]; then
    echo "${ROOT}/external/ipsw/ipsw"
    return
  fi
  if command -v ipsw >/dev/null 2>&1; then
    command -v ipsw
    return
  fi
  echo ""
}

IPSW="$(find_ipsw)"
if [[ -z "${IPSW}" ]]; then
  echo "warning: ipsw not found — install or build external/ipsw" >&2
else
  echo "=== ipsw kernel version ==="
  "${IPSW}" kernel version "${KC}" --json 2>/dev/null || "${IPSW}" kernel version "${KC}" || true

  echo ""
  echo "=== ipsw macho info (arm64 slice) ==="
  "${IPSW}" macho info "${KC}" --json --header --loads --arch arm64 2>/dev/null | head -c 8192 || true
  echo ""

  echo "=== AMFI kext (com.apple.driver.AppleMobileFileIntegrity) ==="
  if "${IPSW}" kernel extract "${KC}" com.apple.driver.AppleMobileFileIntegrity -o /tmp/pp-kpf-amfi 2>/dev/null; then
    ls -la /tmp/pp-kpf-amfi 2>/dev/null || true
    rm -rf /tmp/pp-kpf-amfi
  else
    echo "(extract failed — kext may be embedded in __PRELINK_TEXT only)"
  fi

  if [[ -n "${IPSW_PATH}" && -f "${IPSW_PATH}" ]]; then
    echo ""
    echo "=== ipsw extract --kernel from IPSW ==="
    TMPDIR="$(mktemp -d)"
    "${IPSW}" extract --kernel "${IPSW_PATH}" -o "${TMPDIR}" 2>/dev/null || true
    ls -la "${TMPDIR}" 2>/dev/null || true
    rm -rf "${TMPDIR}"
  fi
fi

echo ""
echo "=== purplepois0n host patchfind ==="
BIN="${ROOT}/build/bin/purplepois0n"
if [[ -x "${BIN}" ]]; then
  "${BIN}" --kernelcache "${KC}"
else
  echo "(build purplepois0n first: make release)"
fi
