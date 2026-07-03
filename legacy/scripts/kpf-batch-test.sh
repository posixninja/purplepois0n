#!/usr/bin/env bash
# Batch-fetch kernelcaches and run kpf-purple against each.
#
# Usage:
#   kpf-batch-test.sh [OUTPUT_DIR]
#
# Env:
#   KPF_BATCH_DEVICES   Space-separated list (default: iPhone10,3 iPhone9,3)
#   KPF_BATCH_VERSIONS  Space-separated iOS versions aligned by index or single for all
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:-/tmp/pp-kpf-fw}"
DEVICES=(${KPF_BATCH_DEVICES:-iPhone10,3 iPhone9,3})
VERSIONS=(${KPF_BATCH_VERSIONS:-14.8 15.7.9})
KPF_TEST="${ROOT}/legacy/kpf-purple/build/kpf-test-purple.macos"
FETCH="${ROOT}/legacy/scripts/kpf-fetch-kernel.sh"

chmod +x "${FETCH}" "${ROOT}/legacy/scripts/kpf-build.sh"
"${ROOT}/legacy/scripts/kpf-build.sh" test

fail=0
pass=0

version_for() {
  local idx="$1"
  if [[ ${#VERSIONS[@]} -eq 1 ]]; then
    echo "${VERSIONS[0]}"
  elif [[ ${idx} -lt ${#VERSIONS[@]} ]]; then
    echo "${VERSIONS[${idx}]}"
  else
    echo "${VERSIONS[0]}"
  fi
}

for i in "${!DEVICES[@]}"; do
  dev="${DEVICES[$i]}"
  ver="$(version_for "${i}")"
  echo ""
  echo "======== ${dev} iOS ${ver} ========"
  if ! "${FETCH}" "${dev}" "${ver}" "${OUT}" >/dev/null; then
    echo "FAIL: fetch ${dev} ${ver}"
    fail=$((fail + 1))
    continue
  fi
  kc="$(find "${OUT}" -path "*${dev}*" -name 'kernelcache.release.*' -type f | head -1)"
  if [[ -z "${kc}" || ! -f "${kc}" ]]; then
    echo "FAIL: no kernelcache for ${dev}"
    fail=$((fail + 1))
    continue
  fi
  log="$(mktemp)"
  if "${KPF_TEST}" "${kc}" >"${log}" 2>&1; then
    if grep -q "KPF: patch summary" "${log}"; then
      summary="$(grep 'KPF: patch summary' "${log}" || true)"
      if grep -q "mode=data-only" "${log}"; then
        if grep -q "snapshot=1\|sbops=1\|data_patches=[1-9]" "${log}"; then
          echo "PASS (data-only): ${kc}"
          echo "  ${summary}"
          pass=$((pass + 1))
        else
          echo "PARTIAL (data-only, no markers): ${kc}"
          echo "  ${summary}"
          pass=$((pass + 1))
        fi
      elif grep -q "amfi=1" "${log}"; then
        echo "PASS: ${kc}"
        echo "  ${summary}"
        pass=$((pass + 1))
      elif grep -qE 'amfi_sha1=1|amfi_execve=1|mac_mount=1|snapshot=1' "${log}"; then
        echo "PARTIAL: ${kc} (no trustcache AMFI; other patches matched)"
        echo "  ${summary}"
        pass=$((pass + 1))
      else
        echo "FAIL: no patches matched ${kc}"
        tail -5 "${log}"
        fail=$((fail + 1))
      fi
    else
      echo "FAIL: no patch summary ${kc}"
      tail -5 "${log}"
      fail=$((fail + 1))
    fi
  else
    echo "FAIL: kpf-test error ${kc}"
    tail -10 "${log}"
    fail=$((fail + 1))
  fi
  rm -f "${log}"
done

echo ""
echo "Batch done: ${pass} passed, ${fail} failed"
[[ "${fail}" -eq 0 ]]
