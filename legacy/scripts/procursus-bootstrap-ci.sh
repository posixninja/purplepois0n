#!/usr/bin/env bash
# Skeleton CI: rebuild a minimal Procursus bootstrap subset for purplepois0n.
# Requires Procursus mirror (legacy/scripts/procursus-mirror.sh) and iOS SDK toolchain.
#
# Usage:
#   PURPLEPOIS0N_IOS_SDK=iphoneos16.5 ./legacy/scripts/procursus-bootstrap-ci.sh
#
# This script does NOT run a full Procursus build — it documents the steps and
# validates the mirror is present. Extend on a Linux/macOS builder with SDKs installed.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PROCURSUS="${PROCURSUS_ROOT:-$ROOT/legacy/modern-era/Procursus}"
SDK="${PURPLEPOIS0N_IOS_SDK:-iphoneos}"
ARCH="${PURPLEPOIS0N_IOS_ARCH:-arm64}"
OUT="${PURPLEPOIS0N_BOOTSTRAP_OUT:-$ROOT/build/bootstrap}"

if [[ ! -d "$PROCURSUS/.git" ]]; then
  echo "Procursus not found. Run: legacy/scripts/procursus-mirror.sh"
  exit 1
fi

mkdir -p "$OUT"

cat >"$OUT/BUILD_PLAN.md" <<EOF
# Procursus bootstrap build plan (purplepois0n)

- SDK: ${SDK}
- Arch: ${ARCH}
- Procursus: ${PROCURSUS}

## Recommended package subset (minimal rootless jb)

- bash, coreutils, apt, dpkg, curl, openssl, ldid, jbctl
- purplepois0n-store default source plist → /var/jb/etc/apt/sources.list.d/

## Steps (on builder host)

1. Follow Procursus BUILDING.md for ${SDK}/${ARCH}.
2. \`make package\` for subset above.
3. Strap to tarball; test on Dopamine device.
4. Publish custom debs: \`purplepois0n --store-publish\`

## purplepois0n post-jb

\`\`\`bash
export PURPLEPOIS0N_JB_HELPER=/path/to/dopamine-cli
export PURPLEPOIS0N_REPO_URL=https://your-cdn/purplepois0n-repo/
purplepois0n --post-jb-pipeline --post-jb-store --normal-ssh -d UDID ...
\`\`\`
EOF

echo "Bootstrap CI skeleton wrote: $OUT/BUILD_PLAN.md"
echo "Extend this script with your Procursus make targets when SDK builder is ready."
