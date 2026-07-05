#!/usr/bin/env bash
# Hardware validation orchestrator (post-MVP gaps plan).
# Offline invariants always run; live device steps skip when PURPLEPOIS0N_DEVICE_UDID is unset.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"
UDID="${PURPLEPOIS0N_DEVICE_UDID:-}"
SCAN_TIMEOUT="${PURPLEPOIS0N_SMOKE_SCAN_TIMEOUT:-15}"

run_scan() {
  if command -v timeout >/dev/null 2>&1; then
    timeout "$SCAN_TIMEOUT" "$@"
  elif command -v gtimeout >/dev/null 2>&1; then
    gtimeout "$SCAN_TIMEOUT" "$@"
  else
    "$@"
  fi
}

echo "=== smoke-hardware-validation: offline gap invariants ==="
"${ROOT}/tests/smoke_mvp_gaps.sh"

echo "=== smoke-hardware-validation: DFU checkm8 wiring (offline) ==="
"${ROOT}/tests/smoke_dfu_jailbreak.sh"

if [[ -z "${UDID}" ]]; then
  echo "=== smoke-hardware-validation: SKIP live device (set PURPLEPOIS0N_DEVICE_UDID) ==="
  echo "Manual checklist: docs/validation/mvp-smoke.md"
  echo "  - already-JB: verify (no store) + All set (store once)"
  echo "  - DFU checkm8: device plan + jailbreak --execute + IPSW/KPF"
  echo "  - store: make smoke-e2e-delegate"
  exit 0
fi

if [[ ! -x "${BIN}" ]]; then
  make -C "${ROOT}" release >/dev/null
fi

export PURPLEPOIS0N_NORMAL_SSH=1

echo "=== smoke-hardware-validation: device plan for ${UDID} ==="
PLAN_JSON="$(run_scan "${BIN}" device plan -d "${UDID}" 2>/dev/null || true)"
if [[ -z "${PLAN_JSON}" ]]; then
  echo "FAIL: device plan returned no output for ${UDID}" >&2
  exit 1
fi
echo "${PLAN_JSON}" | grep -q '"strategy"' || {
  echo "FAIL: device plan missing strategy" >&2
  exit 1
}

STRATEGY="$(echo "${PLAN_JSON}" | python3 -c "import json,sys; d=json.load(sys.stdin); print(d.get('plan',{}).get('strategyId',''))")"
echo "device strategy: ${STRATEGY}"

if [[ "${STRATEGY}" == "normal-already-jailbroken" ]]; then
  echo "--- already-JB verify path (no store sync flags in planner JSON)"
  if echo "${PLAN_JSON}" | grep -qi 'postJbStoreSync.*true'; then
    echo "FAIL: already-JB plan must not default postJbStoreSync" >&2
    exit 1
  fi
  echo "--- external probe only"
  if ! "${BIN}" --external-jailbreak --already-jailbroken --normal-ssh -d "${UDID}"; then
    echo "FAIL: already-jailbroken probe failed" >&2
    exit 1
  fi
  echo "OK: already-JB verify probe passed (run wizard All set separately for store)"
fi

if [[ "${STRATEGY}" == "dfu-checkm8-usb-loader" ]]; then
  echo "OK: DFU checkm8 plan detected — live execute requires IPSW/KPF/plugins (manual)"
fi

if [[ "${STRATEGY}" == "dfu-usbliter8" ]]; then
  echo "OK: DFU usbliter8 plan detected — bootrom-only execute (no Pongo in plan)"
  if echo "${PLAN_JSON}" | grep -q '"useBootDelivery":true'; then
    echo "FAIL: usbliter8 plan must not set useBootDelivery" >&2
    exit 1
  fi
fi

if [[ "${STRATEGY}" == "recovery-ramdisk-chain" ]]; then
  echo "--- recovery-ramdisk-chain plan"
  if ! echo "${PLAN_JSON}" | grep -q 'recoveryChain'; then
    echo "FAIL: recovery plan missing recoveryChain (set PURPLEPOIS0N_IPSW)" >&2
    exit 1
  fi
  echo "OK: recovery plan includes recoveryChain stages"
fi

echo "=== smoke-hardware-validation: e2e delegate store ==="
PURPLEPOIS0N_DEVICE_UDID="${UDID}" "${ROOT}/tests/smoke_e2e_delegate.sh"

echo "smoke-hardware-validation passed (device ${UDID})"
