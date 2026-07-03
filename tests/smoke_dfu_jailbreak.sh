#!/usr/bin/env bash
# Offline smoke for DFU jailbreak orchestration (checkm8 → Pongo chain wiring).
#
# Device recipe (A5–A11 in DFU):
#   make plugins kpf
#   ./build/bin/purplepois0n --dfu-jailbreak --i-understand-jailbreak \
#     --pongo-kpf legacy/kpf-purple/build/purplepois0n-kpf-pongo \
#     --pongo-ramdisk /path/to/raw.dmg
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"

echo "--- build plugins"
make -C "${ROOT}" plugins LIBTATSU=0

if [[ ! -x "${BIN}" ]]; then
  echo "purplepois0n binary missing after plugins build" >&2
  exit 1
fi

echo "--- help documents --dfu-jailbreak"
HELP="$("${BIN}" --help 2>&1)"
echo "${HELP}" | grep -q -- '--dfu-jailbreak'

echo "--- guard requires --i-understand-jailbreak"
GUARD="$("${BIN}" --dfu-jailbreak 2>&1 || true)"
echo "${GUARD}" | grep -q 'i-understand-jailbreak'

echo "--- build kpf-purple"
make -C "${ROOT}" kpf

KPF="${ROOT}/legacy/kpf-purple/build/purplepois0n-kpf-pongo"
if [[ ! -f "${KPF}" ]]; then
  echo "KPF blob missing: ${KPF}" >&2
  exit 1
fi

echo "smoke-dfu-jailbreak passed (offline; attach DFU device for live run)"
