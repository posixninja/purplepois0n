#!/usr/bin/env bash
# Offline rootless layout smoke: fixture tree probe via rootless-bootstrap primitive.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FIXTURE="${ROOT}/tests/fixtures/jbroot_minimal"
BIN="${ROOT}/build/bin/purplepois0n"

make -C "${ROOT}" release >/dev/null

export PURPLEPOIS0N_JBROOT_FIXTURE="${FIXTURE}"
export PURPLEPOIS0N_ROOTLESS=1

LOG="$(mktemp)"
set +e
"${BIN}" --probe-primitive rootless-bootstrap >"${LOG}" 2>&1
RC=$?
set -e

if [[ "${RC}" -ne 0 ]]; then
  echo "rootless-bootstrap failed (exit ${RC})" >&2
  tail -30 "${LOG}"
  exit "${RC}"
fi

if ! grep -q 'dopamine' "${LOG}"; then
  echo "expected dopamine marker in fixture probe" >&2
  tail -20 "${LOG}"
  exit 1
fi

grep -E 'Rootless|jbroot|dopamine|dpkg' "${LOG}" | head -10
echo "OK: rootless layout smoke passed"
rm -f "${LOG}"
