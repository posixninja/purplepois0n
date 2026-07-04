#!/usr/bin/env bash
# external-jailbreak.sh — delegate jailbreak to palera1n or checkra1n (Gen 5 path).
set -euo pipefail

UDID="${PURPLEPOIS0N_DEVICE_UDID:-}"
EXTRA="${PURPLEPOIS0N_EXTERNAL_JAILBREAK_ARGS:-}"

die() { echo "external-jailbreak: $*" >&2; exit 1; }

find_tool() {
  local var="$1"
  shift
  local val="${!var:-}"
  if [[ -n "$val" ]] && [[ -x "$val" ]]; then
    echo "$val"
    return 0
  fi
  for c in "$@"; do
    if [[ -x "$c" ]]; then
      echo "$c"
      return 0
    fi
    if command -v "$c" >/dev/null 2>&1; then
      command -v "$c"
      return 0
    fi
  done
  return 1
}

PALERA1N="$(find_tool PURPLEPOIS0N_PALERA1N palera1n /usr/local/bin/palera1n /opt/homebrew/bin/palera1n || true)"
CHECKRA1N="$(find_tool PURPLEPOIS0N_CHECKRA1N checkra1n /Applications/checkra1n.app/Contents/MacOS/checkra1n /usr/local/bin/checkra1n /opt/homebrew/bin/checkra1n || true)"

if [[ -n "$PALERA1N" ]]; then
  echo "external-jailbreak: palera1n ($PALERA1N)"
  if [[ -z "$EXTRA" ]]; then
    EXTRA="-c -f -l"
  fi
  # shellcheck disable=SC2086
  exec "$PALERA1N" $EXTRA
fi

if [[ -n "$CHECKRA1N" ]]; then
  echo "external-jailbreak: checkra1n ($CHECKRA1N)"
  exec "$CHECKRA1N" -c
fi

die "No palera1n or checkra1n found. Gen6: use Dopamine on device, then --external-jailbreak --already-jailbroken"
