#!/usr/bin/env bash
# Build seed debs and populate ./store + store-publish/
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/build/bin/purplepois0n"
STORE="${PURPLEPOIS0N_STORE_ROOT:-$ROOT/store}"
PUBLISH="${PURPLEPOIS0N_PUBLISH_ROOT:-$ROOT/store-publish}"

make -C "$ROOT" release >/dev/null
"$ROOT/legacy/packages/build-debs.sh"

"$BIN" --store-init --store-root "$STORE"

for deb in "$ROOT/legacy/packages/out"/*.deb; do
  [[ -f "$deb" ]] || continue
  "$BIN" --store-add "$deb" --store-root "$STORE"
done

"$BIN" --store-build --store-root "$STORE"
"$BIN" --store-publish "$PUBLISH" --store-root "$STORE"

echo "Store seeded at $STORE"
echo "Publish tree at $PUBLISH"
