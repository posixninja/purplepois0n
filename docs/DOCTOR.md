# Doctor workflow

One-button **scan → plan → execute** with JSON step events on stdout. Used by the native GUI, web agent, and automation.

See [MVP.md](MVP.md) for the full product flow.

## Probe vs execute

| Mode | CLI | Jailbreak step |
|------|-----|----------------|
| **Probe** | `jailbreak` or `--doctor-run --normal-ssh` | **No** — stops after plan JSON with `"Plan ready"` |
| **Execute** | `jailbreak --execute` | Yes — runs strategy from planner |

Probe mode never spawns external jailbreak helpers or store sync. Use execute only after reviewing the plan and blockers.

## CLI

```bash
# Probe: scan + plan JSON steps (no mutation)
./build/bin/purplepois0n jailbreak
# or: ./build/bin/purplepois0n --doctor-run --normal-ssh

# Plan JSON without step stream
./build/bin/purplepois0n device plan

# Full execute (planner merges options first)
./build/bin/purplepois0n jailbreak --execute
```

Without a USB device, detect fails and the stream ends with `"success": false` — step JSON is still emitted.

## Step protocol

Each line on stdout is a JSON object:

| `type` | Meaning |
|--------|---------|
| `step` | Host workflow step (`detect`, `syringe-connect`, `identify`, `plan`, `jailbreak`) |
| `syringe` | Syringe transport command/response |
| `complete` | Final success/failure |

The **plan** step prints a second line: full `device plan` JSON (device profile + strategy + blockers).

Implementation: `src/DoctorWorkflow.cpp`, `src/DoctorReporter.cpp`.

## Native GUI launchers

| Platform | Launcher |
|----------|----------|
| macOS | `doctors/macos/run-doctor.command` (double-click) |
| Linux | `doctors/linux/run-doctor.sh` |
| Windows | `doctors/windows/run-doctor.bat` |
| Cross-platform | `python3 doctors/doctor_gui.py` |

Set `PURPLEPOIS0N_BIN` if the binary is not at `build/bin/purplepois0n`.

## Agent

```bash
make agent
# Probe
curl -X POST http://127.0.0.1:7749/doctor \
  -H 'Content-Type: application/json' \
  -d '{"execute": false, "udid": "YOUR_UDID"}'
# Execute
curl -X POST http://127.0.0.1:7749/jailbreak \
  -H 'Content-Type: application/json' \
  -d '{"auto": true, "execute": true, "udid": "YOUR_UDID"}'
```

NDJSON stream; agent deduplicates `complete` events when the CLI already emitted one.

## Smoke

```bash
make smoke-doctor
make smoke-agent    # probe must not include jailbreak step
```
