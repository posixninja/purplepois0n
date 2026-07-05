#!/usr/bin/env bash
# Offline smoke: MVP gap A/B/C source invariants (no device required).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "--- gap A: already-JB planner must not default postJbStoreSync"
ALREADY_BLOCK="$(
  awk '/normal-already-jailbroken/{found=1} found && /break;/{print; exit} found' \
    "${ROOT}/src/JailbreakPlanner.cpp"
)"
if echo "${ALREADY_BLOCK}" | grep -q 'postJbStoreSync'; then
  echo "FAIL: normal-already-jailbroken block still sets postJbStoreSync" >&2
  exit 1
fi
echo "OK: already-JB planner block has no postJbStoreSync default"

echo "--- gap A: POST_JB_STORE only for fresh external delegate"
if ! grep -q 'PURPLEPOIS0N_POST_JB_STORE' "${ROOT}/src/JailbreakPlanner.cpp"; then
  echo "FAIL: PURPLEPOIS0N_POST_JB_STORE opt-in missing" >&2
  exit 1
fi
if ! grep -q '!plan.alreadyJailbroken' "${ROOT}/src/JailbreakPlanner.cpp"; then
  echo "FAIL: postJbStoreSync env gate must exclude alreadyJailbroken" >&2
  exit 1
fi
echo "OK: post-jb-store env gated to external delegate only"

echo "--- gap B: dfu-usbliter8 plan has no boot delivery flags"
USB_BLOCK="$(
  awk '/dfu-usbliter8/{found=1} found && /break;/{print; exit} found' \
    "${ROOT}/src/JailbreakPlanner.cpp"
)"
if echo "${USB_BLOCK}" | grep -qE 'useBootDelivery|deliveryRun|pongo\.bootRun'; then
  echo "FAIL: dfu-usbliter8 plan enables boot delivery" >&2
  exit 1
fi
echo "OK: dfu-usbliter8 plan is bootrom-only"

echo "--- gap B: runDfuJailbreak gates Pongo on delivery flags"
if ! grep -q 'options.ramdisk.deliveryRun || options.pongo.bootRun' \
  "${ROOT}/src/Gen0Workflow.cpp"; then
  echo "FAIL: runDfuJailbreak Pongo gate missing" >&2
  exit 1
fi
RUN_PONGO_LINE="$(grep -n 'const bool runPongo' "${ROOT}/src/Gen0Workflow.cpp" | head -1 | cut -d: -f1)"
if [[ -n "${RUN_PONGO_LINE}" ]]; then
  RUN_PONGO_BLOCK="$(sed -n "${RUN_PONGO_LINE},$((RUN_PONGO_LINE + 2))p" "${ROOT}/src/Gen0Workflow.cpp")"
  if echo "${RUN_PONGO_BLOCK}" | grep -q 'jailbreakExecute'; then
    echo "FAIL: runPongo assignment still keys off jailbreakExecute" >&2
    exit 1
  fi
fi
echo "OK: runDfuJailbreak Pongo gated correctly"

echo "--- gap C: pongo-boot writes report on probe failure path"
if ! grep -q 'writeReportToFile' "${ROOT}/src/pongo/PongoWorkflow.cpp"; then
  echo "FAIL: runPongoBoot must write chain report" >&2
  exit 1
fi

BIN="${ROOT}/build/bin/purplepois0n"
if [[ -x "${BIN}" ]]; then
  REPORT="/tmp/pp-smoke-mvp-gaps-pongo.json"
  rm -f "${REPORT}"
  if ! "${BIN}" --pongo-boot --pongo-kpf /tmp/x --pongo-ramdisk /tmp/x.dmg \
    --report "${REPORT}" >/dev/null 2>&1; then
    echo "FAIL: --pongo-boot --report must exit 0 without device" >&2
    exit 1
  fi
  if [[ ! -s "${REPORT}" ]]; then
    echo "FAIL: chain report not written" >&2
    exit 1
  fi
  python3 - "${REPORT}" <<'PY'
import json, sys
with open(sys.argv[1], encoding="utf-8") as f:
    data = json.load(f)
assert isinstance(data.get("reports"), list) and data["reports"], "empty reports"
PY
  echo "OK: --pongo-boot --report exits 0 and writes JSON"
else
  echo "skip: binary not built (make release)"
fi

echo "smoke-mvp-gaps passed"
