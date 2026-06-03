#!/usr/bin/env bash
# Initialize git submodules and build bundled ipsw.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${ROOT}"

if [[ ! -d .git ]]; then
  echo "Not a git repository. Run: git init && git submodule add https://github.com/blacktop/ipsw.git external/ipsw"
  exit 1
fi

git submodule update --init --recursive external/ipsw

if ! command -v go >/dev/null 2>&1; then
  echo "Go is required to build ipsw. Install Go or set PURPLEPOIS0N_IPSW to a built binary."
  exit 1
fi

echo "Building external/ipsw..."
make -C external/ipsw build

if [[ -x external/ipsw/ipsw ]]; then
  echo "OK: ${ROOT}/external/ipsw/ipsw"
  external/ipsw/ipsw version || true
else
  echo "Build finished but external/ipsw/ipsw is missing."
  exit 1
fi
