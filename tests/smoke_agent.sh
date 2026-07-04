#!/usr/bin/env bash
# smoke_agent.sh — localhost agent health + doctor NDJSON stream (offline)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"
AGENT="${ROOT}/ui/agent/purple_agent.py"
PORT="${PURPLEPOIS0N_AGENT_PORT:-7749}"
BASE="http://127.0.0.1:${PORT}"

make -C "$ROOT" release >/dev/null

if ! command -v python3 >/dev/null 2>&1; then
  echo "SKIP: python3 not found"
  exit 0
fi

chmod +x "$AGENT"
PURPLEPOIS0N_BIN="$BIN" python3 "$AGENT" &
PID=$!
cleanup() { kill "$PID" 2>/dev/null || true; }
trap cleanup EXIT

for _ in $(seq 1 30); do
  if curl -sf "$BASE/health" >/dev/null 2>&1; then
    break
  fi
  sleep 0.2
done

HEALTH="$(curl -sf "$BASE/health")"
echo "$HEALTH" | grep -q '"ok": true' || {
  echo "FAIL: agent health not ok: $HEALTH"
  exit 1
}
echo "$HEALTH" | grep -q '"capabilities"' || {
  echo "FAIL: expected capabilities in health"
  exit 1
}

DEVICES="$(curl -sf "$BASE/devices")"
echo "$DEVICES" | grep -q '"devices"' || {
  echo "FAIL: expected devices array"
  exit 1
}

STORE_PKGS="$(curl -sf "$BASE/store/packages" 2>/dev/null || echo '')"
echo "$STORE_PKGS" | grep -q 'Package:' || {
  echo "FAIL: /store/packages did not return Packages index"
  echo "$STORE_PKGS" | head -5
  exit 1
}

if [[ -n "${PURPLEPOIS0N_DEVICE_UDID:-}" ]]; then
  PLAN="$(curl -sf "$BASE/device/plan?udid=${PURPLEPOIS0N_DEVICE_UDID}" 2>/dev/null || echo '{}')"
  echo "$PLAN" | grep -q '"plan"' || {
    echo "FAIL: /device/plan failed for PURPLEPOIS0N_DEVICE_UDID"
    echo "$PLAN"
    exit 1
  }
else
  PLAN="$(curl -sf "$BASE/device/plan" 2>/dev/null || echo '{}')"
  echo "$PLAN" | grep -q '"plan"' || echo "WARN: /device/plan unavailable (no device?)"
fi

OUT="$(curl -sf -X POST -H 'Content-Type: application/json' -d '{"execute":false}' "$BASE/doctor" || true)"
echo "$OUT" | grep -q '"type"[[:space:]]*:[[:space:]]*"step"' || {
  echo "FAIL: expected doctor step JSON"
  echo "$OUT" | head -5
  exit 1
}
echo "$OUT" | grep -q '"type"[[:space:]]*:[[:space:]]*"complete"' || {
  echo "FAIL: expected doctor complete JSON"
  exit 1
}
# Probe-only doctor must not enter jailbreak step.
if echo "$OUT" | grep -q '"id"[[:space:]]*:[[:space:]]*"jailbreak"'; then
  echo "FAIL: doctor probe should not run jailbreak step"
  exit 1
fi

JB_OUT="$(curl -sf -X POST -H 'Content-Type: application/json' \
  -d '{"auto":true,"execute":false}' "$BASE/jailbreak" 2>/dev/null || true)"
echo "$JB_OUT" | grep -q '"type"[[:space:]]*:[[:space:]]*"complete"' || {
  echo "FAIL: /jailbreak auto probe expected complete JSON"
  echo "$JB_OUT" | head -5
  exit 1
}

echo "OK: agent smoke passed"
