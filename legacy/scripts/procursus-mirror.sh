#!/usr/bin/env bash
# Mirror Procursus into legacy/modern-era/ for bootstrap study and custom package builds.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DEST="$ROOT/legacy/modern-era/Procursus"

if [[ -d "$DEST/.git" ]]; then
  echo "Procursus mirror exists: $DEST"
  echo "  git -C $DEST pull --ff-only"
  exit 0
fi

mkdir -p "$(dirname "$DEST")"
git clone --depth 1 https://github.com/ProcursusTeam/Procursus.git "$DEST"
echo "Cloned Procursus to $DEST"
echo ""
echo "Next: build on Linux/macOS SDK per Procursus README."
echo "Wire default repo: export PURPLEPOIS0N_REPO_URL=https://your-host/"
