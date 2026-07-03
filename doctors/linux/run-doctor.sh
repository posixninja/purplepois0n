#!/usr/bin/env bash
# Linux launcher for PurpleDoctor.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
exec python3 "$ROOT/doctors/doctor_gui.py"
