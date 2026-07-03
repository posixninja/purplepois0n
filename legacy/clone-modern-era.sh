#!/usr/bin/env bash
# Clone Gen 6 (modern era) study mirrors into legacy/modern-era/.
# Idempotent: skips directories that already contain .git
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
DEST="$ROOT/modern-era"
mkdir -p "$DEST"

clone_repo() {
  local url="$1"
  local name="$2"
  local branch="${3:-}"
  local dir="$DEST/$name"
  if [[ -d "$dir/.git" ]]; then
    echo "skip (exists): $name"
    return 0
  fi
  echo "clone: $name"
  if [[ -n "$branch" ]]; then
    git clone --depth 1 --branch "$branch" "$url" "$dir"
  else
    git clone --depth 1 "$url" "$dir"
  fi
}

# P0 — exploit modules integrated in Dopamine picker
clone_repo "https://github.com/opa334/Dopamine.git" "Dopamine" "2.x"
clone_repo "https://github.com/opa334/kfd.git" "kfd-opa334"
clone_repo "https://github.com/felix-pb/kfd.git" "kfd-felix-pb"
clone_repo "https://github.com/opa334/XPF.git" "XPF"
clone_repo "https://github.com/0x36/weightBufs.git" "weightBufs"
clone_repo "https://github.com/potmdehex/multicast_bytecopy.git" "multicast_bytecopy"
clone_repo "https://github.com/opa334/darksword-kexploit.git" "darksword-kexploit"

# P1 — bootstrap, install path, kernelcache fetch, PongoOS KPF dev
clone_repo "https://github.com/checkra1n/PongoOS.git" "PongoOS"
clone_repo "https://github.com/opa334/TrollStore.git" "TrollStore"
clone_repo "https://github.com/opa334/libroot.git" "libroot"
clone_repo "https://github.com/Siguza/libkrw.git" "libkrw"
clone_repo "https://github.com/alfiecg24/libgrabkernel2.git" "libgrabkernel2"
clone_repo "https://github.com/ProcursusTeam/Procursus.git" "Procursus"
clone_repo "https://github.com/tealbathingsuit/ellekit.git" "ellekit"

# checkm8 era — PongoOS + KPF patchfinder (offline kpf-test)
clone_repo "https://github.com/checkra1n/PongoOS.git" "PongoOS"

# P2 — lineage + community forks / analysis
clone_repo "https://github.com/opa334/Fugu15.git" "Fugu15"
clone_repo "https://github.com/wh1te4ever/multicast_bytecopy_A9.git" "multicast_bytecopy_A9"
clone_repo "https://github.com/AntonioCiolino/DarkSword-Analysis.git" "DarkSword-Analysis"

echo "Done. Mirrors under: $DEST"
