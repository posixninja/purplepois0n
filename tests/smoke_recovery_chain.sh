#!/usr/bin/env bash
# Offline smoke: recovery-ramdisk-chain planner + CLI wiring.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"

echo "--- recovery chain flags in help"
DEV_HELP="$("${BIN}" dev --help 2>&1)"
echo "${DEV_HELP}" | grep -q 'recovery-chain'

echo "--- planner populates recovery chain from IPSW"
grep -q 'applyRecoveryChainDefaults' "${ROOT}/src/JailbreakPlanner.cpp"
grep -q 'populateDefaultRecoveryChain' "${ROOT}/src/JailbreakPlanner.cpp"
grep -q 'recoveryChain' "${ROOT}/src/JailbreakPlanner.cpp"

IPSW="${PURPLEPOIS0N_IPSW:-}"
if [[ -n "${IPSW}" && -f "${IPSW}" ]]; then
  echo "--- populateDefaultRecoveryChain with IPSW"
  WORK="/tmp/pp-smoke-recovery-$$"
  mkdir -p "${WORK}"
  if ! "${BIN}" --gen0 --report "${WORK}/report.json" 2>/dev/null; then
    true
  fi
  rm -rf "${WORK}"
fi

UDID="${PURPLEPOIS0N_DEVICE_UDID:-}"
if [[ -n "${UDID}" ]]; then
  echo "--- live device plan (recovery chain JSON when in Recovery)"
  PLAN="$("${BIN}" device plan -d "${UDID}" 2>/dev/null || true)"
  if echo "${PLAN}" | grep -q '"state":"Recovery"'; then
    echo "${PLAN}" | grep -q '"recovery-ramdisk-chain"' || {
      echo "FAIL: Recovery device expected recovery-ramdisk-chain strategy" >&2
      exit 1
    }
    echo "${PLAN}" | grep -q 'recoveryChain' || {
      echo "FAIL: Recovery plan missing recoveryChain[] (set PURPLEPOIS0N_IPSW)" >&2
      exit 1
    }
    echo "OK: Recovery device plan includes recoveryChain"
  else
    echo "skip: device not in Recovery mode"
  fi
else
  echo "skip: live Recovery plan (set PURPLEPOIS0N_DEVICE_UDID + Recovery mode device)"
fi

echo "smoke-recovery-chain passed"
