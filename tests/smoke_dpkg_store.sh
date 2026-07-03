#!/usr/bin/env bash
# smoke_dpkg_store.sh — host dpkg repo init/build/publish and dpkg-store primitive (offline)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"
STORE="${TMPDIR:-/tmp}/pp-dpkg-store-smoke-$$"
PUBLISH="${TMPDIR:-/tmp}/pp-dpkg-store-publish-$$"

cleanup() {
  rm -rf "$STORE" "$PUBLISH"
}
trap cleanup EXIT

make -C "$ROOT" release >/dev/null

"$BIN" --store-init --store-root "$STORE"
"$BIN" --store-build --store-root "$STORE"

test -d "$STORE/pool/main"
test -f "$STORE/Packages"
test -f "$STORE/dists/purplepois0n/Release"
test -f "$STORE/dists/purplepois0n/main/binary-iphoneos-arm64/Packages"

if command -v gzip >/dev/null 2>&1; then
  test -f "$STORE/dists/purplepois0n/main/binary-iphoneos-arm64/Packages.gz"
fi

if command -v dpkg-deb >/dev/null 2>&1; then
  DEB_WORK="$STORE/deb-work"
  mkdir -p "$DEB_WORK/DEBIAN"
  cat >"$DEB_WORK/DEBIAN/control" <<EOF
Package: purplepois0n-smoke
Version: 0.0.1
Architecture: iphoneos-arm64
Maintainer: purplepois0n
Description: smoke test package
EOF
  DEB_PATH="$STORE/purplepois0n-smoke_0.0.1_iphoneos-arm64.deb"
  dpkg-deb -b "$DEB_WORK" "$DEB_PATH" >/dev/null
  PURPLEPOIS0N_STORE_ROOT="$STORE" "$BIN" --store-add "$DEB_PATH" --store-root "$STORE"
  grep -q '^Package: purplepois0n-smoke$' "$STORE/Packages"
  grep -q 'pool/main/p/purplepois0n-smoke/' "$STORE/Packages"
fi

"$BIN" --store-publish "$PUBLISH" --store-root "$STORE"
test -d "$PUBLISH/pool/main"
test -f "$PUBLISH/dists/purplepois0n/Release"
test -f "$PUBLISH/README-hosting.txt"

PURPLEPOIS0N_STORE_ROOT="$STORE" "$BIN" --probe-primitive dpkg-store

echo "smoke_dpkg_store: OK"
