#!/usr/bin/env bash
# Sync purplepois0n kpf-purple into PongoOS mirror and build kpf-test / Pongo module.
#
# Env:
#   PONGO_ROOT          Override PongoOS path (default: legacy/modern-era/PongoOS)
#   KPF_PURPLE_KERNELCACHE  Kernelcache for optional post-build smoke
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
LEGACY="$(cd "$(dirname "$0")/.." && pwd)"
KPF_SRC="${LEGACY}/kpf-purple"
PONGO_ROOT="${PONGO_ROOT:-${LEGACY}/modern-era/PongoOS}"
BUILD="${KPF_SRC}/build"

sync_pongoos() {
  if [[ ! -d "${PONGO_ROOT}/.git" ]]; then
    echo "Cloning PongoOS into ${PONGO_ROOT}..."
    mkdir -p "$(dirname "${PONGO_ROOT}")"
    git clone --depth 1 "https://github.com/checkra1n/PongoOS.git" "${PONGO_ROOT}"
  fi
}

sync_kpf_purple() {
  sync_pongoos
  local dest="${PONGO_ROOT}/checkra1n/kpf-purple"
  mkdir -p "${dest}"
  rsync -a --delete \
    --exclude build \
    "${KPF_SRC}/" "${dest}/"

  # Host test harness from upstream kpf-test (not vendored in full — copy once).
  if [[ ! -f "${KPF_SRC}/kpf-test-main.c" ]]; then
    if [[ -f "${PONGO_ROOT}/checkra1n/kpf-test/main.c" ]]; then
      cp "${PONGO_ROOT}/checkra1n/kpf-test/main.c" "${KPF_SRC}/kpf-test-main.c"
      echo "Copied kpf-test-main.c from PongoOS mirror"
    else
      echo "error: kpf-test-main.c missing and PongoOS kpf-test not present" >&2
      exit 1
    fi
  fi
}

build_kpf_test() {
  sync_kpf_purple
  make -C "${KPF_SRC}" PONGO_ROOT="${PONGO_ROOT}" kpf-test.macos
  echo "Built: ${BUILD}/kpf-test-purple.macos"
}

build_pongo_module() {
  sync_kpf_purple
  make -C "${KPF_SRC}" PONGO_ROOT="${PONGO_ROOT}" purplepois0n-kpf-pongo
  echo "Built: ${BUILD}/purplepois0n-kpf-pongo"
}

cmd="${1:-all}"
case "${cmd}" in
  sync)
    sync_kpf_purple
    ;;
  test)
    build_kpf_test
    ;;
  module)
    build_pongo_module
    ;;
  all)
    build_kpf_test
    build_pongo_module
    ;;
  *)
    echo "Usage: $0 {sync|test|module|all}" >&2
    exit 1
    ;;
esac

if [[ -n "${KPF_PURPLE_KERNELCACHE:-}" && -x "${BUILD}/kpf-test-purple.macos" ]]; then
  echo "--- quick kpf-test on ${KPF_PURPLE_KERNELCACHE}"
  "${BUILD}/kpf-test-purple.macos" "${KPF_PURPLE_KERNELCACHE}" || true
fi
