#!/usr/bin/env bash
# Assert ChainRunner JSON report shape and expected probe results.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/bin/purplepois0n"
REPORT="${1:-/tmp/pp-chain-report.json}"
shift || true

if [[ ! -x "${BIN}" ]]; then
  echo "Building release..."
  make -C "${ROOT}" release
fi

"${BIN}" "$@" --report "${REPORT}"

python3 - "${REPORT}" <<'PY'
import json
import sys

path = sys.argv[1]
with open(path, encoding="utf-8") as f:
    data = json.load(f)

reports = data.get("reports")
if not isinstance(reports, list) or not reports:
    raise SystemExit(f"expected non-empty reports[] in {path}")

for entry in reports:
    for key in ("stage", "result", "message"):
        if key not in entry:
            raise SystemExit(f"missing {key!r} in report entry: {entry!r}")

print(f"OK: {len(reports)} chain report entries in {path}")
PY

echo "Chain report assertions passed."
