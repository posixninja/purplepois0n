#!/usr/bin/env bash
# Download and extract a kernelcache via ipsw for KPF development.
#
# Usage:
#   kpf-fetch-kernel.sh DEVICE [VERSION] [OUTPUT_DIR]
#
# Examples:
#   kpf-fetch-kernel.sh iPhone10,3 14.8
#   kpf-fetch-kernel.sh iPhone9,1 15.7.9 /tmp/pp-kpf-fw
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DEVICE="${1:-iPhone10,3}"
VERSION="${2:-14.8}"
OUT="${3:-/tmp/pp-kpf-fw}"

find_ipsw() {
  if [[ -n "${PURPLEPOIS0N_IPSW:-}" && -x "${PURPLEPOIS0N_IPSW}" ]]; then
    echo "${PURPLEPOIS0N_IPSW}"
    return
  fi
  if [[ -x "${ROOT}/external/ipsw/ipsw" ]]; then
    echo "${ROOT}/external/ipsw/ipsw"
    return
  fi
  command -v ipsw 2>/dev/null || true
}

IPSW="$(find_ipsw)"
if [[ -z "${IPSW}" ]]; then
  echo "ipsw not found — build external/ipsw or set PURPLEPOIS0N_IPSW" >&2
  exit 1
fi

mkdir -p "${OUT}"
echo "Fetching ${DEVICE} iOS ${VERSION} kernelcache → ${OUT}"
"${IPSW}" download ipsw --device "${DEVICE}" --version "${VERSION}" --kernel -o "${OUT}" -y

KC="$(find "${OUT}" -name 'kernelcache.release.*' -type f | head -1)"
if [[ -z "${KC}" ]]; then
  echo "kernelcache not found under ${OUT}" >&2
  exit 1
fi

echo ""
echo "KERNELCACHE=${KC}"
echo "Run: KPF_PURPLE_KERNELCACHE=${KC} ${ROOT}/tests/smoke_kpf_test.sh"
