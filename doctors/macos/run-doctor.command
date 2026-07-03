#!/bin/bash
# macOS launcher for PurpleDoctor (double-click in Finder or run from Terminal).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
exec python3 "$ROOT/doctors/doctor_gui.py"
