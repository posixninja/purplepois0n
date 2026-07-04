#!/usr/bin/env bash
# Publish store and print HTTPS deployment instructions.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PUBLISH="${1:-$ROOT/store-publish}"
HOST="${PURPLEPOIS0N_REPO_URL:-https://YOUR_HOST/purplepois0n-repo/}"
HOST="${HOST%/}/"

if [[ ! -d "$PUBLISH/dists" ]]; then
  echo "error: run legacy/scripts/seed-store.sh first" >&2
  exit 1
fi

if [[ -f "$PUBLISH/README-hosting.txt" ]]; then
  cat "$PUBLISH/README-hosting.txt"
  exit 0
fi

cat <<EOF
purplepois0n HTTPS repo deploy
==============================

Publish directory: $PUBLISH

1. Upload entire directory to static HTTPS host:

   rsync -avz "$PUBLISH/" user@server:/var/www/purplepois0n-repo/

2. nginx location block:

   location /purplepois0n-repo/ {
     alias /var/www/purplepois0n-repo/;
     autoindex off;
   }

3. Device apt source (/var/jb/etc/apt/sources.list.d/purplepois0n.list):

   deb [trusted=yes] ${HOST} purplepois0n main

4. On device:

   apt update
   apt install purplepois0n-smoke purplepois0n-zebra purplepois0n-sources

5. Dev SSH file mirror (alternative to HTTPS):

   ./build/bin/purplepois0n store sync --store-sync-mode file --normal-ssh -d YOUR_UDID

Set PURPLEPOIS0N_REPO_URL before seed-store.sh to bake URL into purplepois0n-sources.
EOF
