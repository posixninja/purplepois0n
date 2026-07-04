#!/usr/bin/env bash
# install-on-device.sh — push purplepois0n-store to a jailbroken device and install packages
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${PURPLEPOIS0N_BIN:-$ROOT/build/bin/purplepois0n}"
STORE="${PURPLEPOIS0N_STORE_ROOT:-$ROOT/store}"
UDID="${PURPLEPOIS0N_DEVICE_UDID:-}"
INSTALL_PKG="${1:-purplepois0n-zebra}"
ALSO_SMOKE="${INSTALL_SMOKE:-1}"

export PURPLEPOIS0N_NORMAL_SSH=1
export PURPLEPOIS0N_RAMDISK_AUTO_IPROXY=1

die() { echo "error: $*" >&2; exit 1; }

[[ -x "$BIN" ]] || die "build first: make release"

echo "==> Ensuring store catalog is seeded"
if [[ ! -d "$STORE/pool/main/p/purplepois0n-smoke" ]]; then
  "$ROOT/legacy/scripts/seed-store.sh"
fi

echo "==> Looking for connected devices"
mapfile -t UDIDS < <(idevice_id -l 2>/dev/null || true)
if [[ ${#UDIDS[@]} -eq 0 ]]; then
  die "No iPhone/iPad detected.

  1. Plug in USB cable
  2. Unlock the device
  3. Tap Trust on the Trust This Computer prompt
  4. Quit Finder if it grabs the device
  5. Re-run: legacy/scripts/install-on-device.sh"
fi

if [[ -z "$UDID" ]]; then
  if [[ ${#UDIDS[@]} -eq 1 ]]; then
    UDID="${UDIDS[0]}"
  else
    echo "Multiple devices — set PURPLEPOIS0N_DEVICE_UDID to one of:"
    printf '  %s\n' "${UDIDS[@]}"
    exit 1
  fi
fi

echo "==> Device UDID: $UDID"
NAME="$(ideviceinfo -u "$UDID" -k DeviceName 2>/dev/null || echo unknown)"
IOS="$(ideviceinfo -u "$UDID" -k ProductVersion 2>/dev/null || echo unknown)"
echo "    $NAME · iOS $IOS"

echo "==> Probing jailbreak (/var/jb) over SSH"
if ! "$BIN" --rootless-probe --normal-ssh -d "$UDID"; then
  die "Rootless probe failed. The phone must be jailbroken (Dopamine/palera1n) with SSH reachable.
  Try: dopamine app → enable SSH, or palera1n bootstrap.
  Default SSH user is often 'mobile' or 'root' with password 'alpine' (change it!)."
fi

echo "==> Syncing store to device"
"$BIN" store sync -d "$UDID" --store-root "$STORE"

if [[ "$ALSO_SMOKE" == "1" ]]; then
  echo "==> Installing smoke test package"
  "$BIN" store install purplepois0n-smoke -d "$UDID" --store-root "$STORE" || true
fi

echo "==> Installing $INSTALL_PKG"
"$BIN" store install "$INSTALL_PKG" -d "$UDID" --store-root "$STORE"

echo ""
echo "Done. On device:"
echo "  • apt source: /var/jb/etc/apt/sources.list.d/purplepois0n.list"
echo "  • store files: /var/jb/var/mobile/purplepois0n-store/"
echo "  • launcher: /var/jb/usr/bin/purplepois0n-store (if purplepois0n-zebra installed)"
echo ""
echo "Or open the web UI: make agent && make web-dev → Jailbreak wizard step 5"
