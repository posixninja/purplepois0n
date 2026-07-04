#!/usr/bin/env bash
# smoke_dpkg_store.sh — host dpkg repo init/build/publish and dpkg-store primitive (offline)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"
STORE="${TMPDIR:-/tmp}/pp-dpkg-store-smoke-$$"
PUBLISH="${TMPDIR:-/tmp}/pp-dpkg-store-publish-$$"
DEB_DIR="$STORE/debs"

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

mkdir -p "$DEB_DIR"
python3 "$ROOT/legacy/packages/build_debs.py" --out-dir "$DEB_DIR" --zebra-sources none >/dev/null
for deb in "$DEB_DIR"/*.deb; do
  [[ -f "$deb" ]] || continue
  "$BIN" --store-add "$deb" --store-root "$STORE"
done
grep -q '^Package: purplepois0n-smoke$' "$STORE/Packages"
grep -q 'pool/main/p/purplepois0n-smoke/' "$STORE/Packages"
grep -q '^Package: purplepois0n-zebra$' "$STORE/Packages"
! grep -q '^Package: purplepois0n-sources$' "$STORE/Packages" || {
  echo "FAIL: dev build should not include purplepois0n-sources by default"
  exit 1
}

HTTPS_DEB_DIR="${TMPDIR:-/tmp}/pp-dpkg-https-debs-$$"
mkdir -p "$HTTPS_DEB_DIR"
PURPLEPOIS0N_REPO_URL=https://cdn.example.com/purplepois0n-repo/ \
  python3 "$ROOT/legacy/packages/build_debs.py" --out-dir "$HTTPS_DEB_DIR" --zebra-sources https >/dev/null
test -f "$HTTPS_DEB_DIR/purplepois0n-sources_1.0.0_iphoneos-arm64.deb"
rm -rf "$HTTPS_DEB_DIR"

"$BIN" --store-publish "$PUBLISH" --store-root "$STORE"
test -d "$PUBLISH/pool/main"
test -f "$PUBLISH/dists/purplepois0n/Release"
test -f "$PUBLISH/README-hosting.txt"
grep -q 'location /purplepois0n-repo/' "$PUBLISH/README-hosting.txt"
grep -q 'purplepois0n-repo/' "$PUBLISH/README-hosting.txt"

PURPLEPOIS0N_STORE_ROOT="$STORE" "$BIN" --probe-primitive dpkg-store

"$BIN" store build --store-root "$STORE" >/dev/null
grep -q '^Package: purplepois0n-smoke$' "$STORE/Packages" || {
  echo "FAIL: subcommand store build"
  exit 1
}

echo "smoke_dpkg_store: OK"
