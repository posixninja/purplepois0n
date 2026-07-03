#!/usr/bin/env bash
# Fork Zebra into legacy/zebra-purple/ with purplepois0n default repo URL.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DEST="$ROOT/legacy/zebra-purple"
REPO_URL="${PURPLEPOIS0N_REPO_URL:-https://example.com/purplepois0n-repo/}"
UPSTREAM="${ZEBRA_UPSTREAM:-https://github.com/zbra-dev/Zebra.git}"

write_overlay() {
  CONFIG="$DEST/purplepois0n-repo.plist"
  cat >"$CONFIG" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key>
  <string>purplepois0n</string>
  <key>URL</key>
  <string>${REPO_URL}</string>
  <key>Suite</key>
  <string>purplepois0n</string>
  <key>Component</key>
  <string>main</string>
</dict>
</plist>
EOF

  README="$DEST/PURPLEPOIS0N.md"
  cat >"$README" <<EOF
# Zebra — purplepois0n fork overlay

Default repo URL: \`${REPO_URL}\`

## Clone upstream (if not present)

\`\`\`bash
git clone --depth 1 ${UPSTREAM} legacy/zebra-purple-upstream
# merge overlay files into your fork
\`\`\`

## Integrate default source

1. Open Zebra Xcode project.
2. Add \`purplepois0n-repo.plist\` to the app bundle resources.
3. Import URL on first launch or hardcode in sources manager (libroot / \`/var/jb\` paths).
4. Build rootless .deb and publish:

\`\`\`bash
purplepois0n --store-add Zebra.deb --store-build --store-publish
\`\`\`

Re-run \`legacy/scripts/zebra-fork-setup.sh\` after changing \`PURPLEPOIS0N_REPO_URL\`.
EOF
  echo "Wrote $CONFIG and $README"
}

mkdir -p "$DEST"

if [[ -d "$DEST/.git" ]]; then
  echo "Zebra fork exists: $DEST"
elif git clone --depth 1 "$UPSTREAM" "$DEST" 2>/dev/null; then
  echo "Cloned Zebra to $DEST"
else
  echo "warn: could not clone $UPSTREAM — writing overlay only under $DEST"
  touch "$DEST/.purplepois0n-overlay"
fi

write_overlay
echo "Open legacy/zebra-purple in Xcode (or merge overlay into upstream clone)."
