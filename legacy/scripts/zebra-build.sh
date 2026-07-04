#!/usr/bin/env bash
# Build purplepois0n store client .deb (Zebra fork or stub when upstream unavailable).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DEST="$ROOT/legacy/zebra-purple"
UPSTREAM_DIR="$ROOT/legacy/zebra-purple-upstream"
OUT_DIR="${PURPLEPOIS0N_DEB_OUT:-$ROOT/legacy/packages/out}"
REPO_URL="${PURPLEPOIS0N_REPO_URL:-https://example.com/purplepois0n-repo/}"
PKG_NAME="purplepois0n-zebra"
PKG_VERSION="${PURPLEPOIS0N_ZEBRA_VERSION:-1.0.0}"
ARCH="iphoneos-arm64"

mkdir -p "$OUT_DIR"

ensure_fork() {
  if [[ ! -f "$DEST/purplepois0n-repo.plist" ]]; then
    PURPLEPOIS0N_REPO_URL="$REPO_URL" "$ROOT/legacy/scripts/zebra-fork-setup.sh"
  fi
}

build_stub_deb() {
  python3 "$ROOT/legacy/packages/build_debs.py" --out-dir "$OUT_DIR" \
    --repo-url "${REPO_URL}" --zebra-version "${PKG_VERSION}" | tail -1
}

build_upstream_deb() {
  local src="$UPSTREAM_DIR"
  if [[ ! -d "$src/.git" && ! -d "$src" ]]; then
    return 1
  fi
  if [[ -f "$src/Makefile" ]] && command -v make >/dev/null 2>&1; then
    (cd "$src" && make package FINALPACKAGE=1 2>/dev/null) || return 1
    local deb
    deb="$(find "$src/packages" -name '*.deb' 2>/dev/null | head -1)"
    if [[ -n "$deb" && -f "$deb" ]]; then
      cp "$deb" "$OUT_DIR/"
      echo "$OUT_DIR/$(basename "$deb")"
      return 0
    fi
  fi
  if [[ -d "$src/Zebra.xcodeproj" ]] && command -v xcodebuild >/dev/null 2>&1; then
    (cd "$src" && xcodebuild -scheme Zebra -configuration Release -sdk iphoneos build) || return 1
    echo "warn: xcodebuild succeeded but .deb packaging requires Theos/finalpackage step" >&2
  fi
  return 1
}

ensure_fork

if deb="$(build_upstream_deb 2>/dev/null)"; then
  echo "Built upstream Zebra: $deb"
  exit 0
fi

deb="$(build_stub_deb)"
echo "Built stub store client: $deb"
echo "Tip: clone Zebra to legacy/zebra-purple-upstream and install Theos for a full app build."
