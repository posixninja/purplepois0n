# Localhost agent API

HTTP bridge between the web UI (or automation) and the native `purplepois0n` CLI.

**Run:** `make agent` (default `http://127.0.0.1:7749`)

Implementation: [ui/agent/purple_agent.py](../ui/agent/purple_agent.py)

The agent sets `PURPLEPOIS0N_NORMAL_SSH=1` and `PURPLEPOIS0N_RAMDISK_AUTO_IPROXY=1` for device plan, doctor, and jailbreak subprocesses so Normal-mode SSH probes work without extra host config.

---

## GET `/health`

Host readiness for the web UI.

```json
{
  "ok": true,
  "bin": "/path/to/build/bin/purplepois0n",
  "version": "0.3.0",
  "storeRoot": "/path/to/store",
  "plugins": true,
  "capabilities": {
    "plugins": true,
    "doctor": true,
    "store": true,
    "normalSsh": true,
    "kpf": {
      "built": true,
      "moduleBuilt": true,
      "testBuilt": true,
      "module": "legacy/kpf-purple/build/kpf-purple.macos"
    }
  }
}
```

`capabilities` mirrors `./build/bin/purplepois0n --capabilities`.

---

## GET `/devices`

List USB devices (Normal / Recovery / DFU).

```json
{
  "devices": [
    {
      "udid": "...",
      "state": "normal",
      "ecid": "0x...",
      "cpid": "0x8015",
      "deviceType": "iPhone12,1",
      "firmware": "15.0"
    }
  ]
}
```

---

## GET `/device/plan?udid=`

**Planner output** — primary input for automation / future LLM orchestration.

Returns the same JSON as `purplepois0n device plan -d UDID`:

```json
{
  "device": {
    "state": "dfu",
    "cpid": "0x8015",
    "soc": "A11",
    "iosVersion": "15.0",
    "era": "gen6"
  },
  "plan": {
    "strategy": "dfu-checkm8-usb-loader",
    "summary": "checkm8 bootrom → USB loader boot (module + ramdisk)",
    "canExecute": true,
    "blockers": [],
    "ramdiskSource": "ipsw-pack",
    "ipsw": "/path/to/firmware.ipsw",
    "alreadyJailbroken": false
  }
}
```

Without a connected device: HTTP 404 and `{ "error": "..." }`.

---

## POST `/doctor`

NDJSON stream of doctor steps (same as `--doctor-run`).

**Body:**

```json
{ "execute": false, "udid": "optional" }
```

| Field | Effect |
|-------|--------|
| `execute: true` | Adds `--jailbreak-execute --i-understand-jailbreak` |
| `udid` | `-d UDID` (falls back to `PURPLEPOIS0N_DEVICE_UDID`) |

**Stream events:**

| `type` | Fields |
|--------|--------|
| `step` | `id`, `phase` (`request`/`response`), `success`, `detail` |
| `syringe` | `transport`, `command`, `success` |
| `complete` | `success`, `detail` |

The **plan** step is followed by a second stdout line in the CLI; the agent forwards the full stream including embedded plan JSON when present.

---

## POST `/jailbreak`

Primary web wizard entry. **Recommended body for auto flow:**

```json
{
  "auto": true,
  "execute": true,
  "post_jb_store": false,
  "udid": "..."
}
```

| Field | Effect |
|-------|--------|
| `auto: true` | `--doctor-run --normal-ssh` (planner-driven) |
| `execute: true` | Mutating execute path |
| `post_jb_store: true` | `--post-jb-store` after external/already-jailbroken paths |
| `already_jailbroken` | Legacy direct `--external-jailbreak --already-jailbroken` (prefer `auto`) |
| `experimental_dfu: true` | `--dfu-jailbreak` when device is DFU |

Response: `application/x-ndjson` chunked stream. The web wizard uses this path exclusively (not `experimental_dfu`).

---

## Experimental / direct endpoints

These bypass the doctor planner. The **web Jailbreak wizard does not call them** — it uses `POST /jailbreak` with `auto: true`.

| Method | Path | Purpose |
|--------|------|---------|
| POST | `/checkm8` | Direct `--checkm8` stream |
| POST | `/dfu-jailbreak` | Direct `--dfu-jailbreak --i-understand-jailbreak` |
| POST | `/external-jailbreak` | Direct `--external-jailbreak` (delegate or already-jailbroken) |

---

## POST `/external-jailbreak`

Direct external delegate (bypasses doctor planner):

```json
{
  "udid": "...",
  "already_jailbroken": true,
  "post_jb_store": true,
  "post_jb_store_install": "purplepois0n-zebra"
}
```

---

## Store endpoints

| Method | Path | Body / query | Response |
|--------|------|--------------|----------|
| GET | `/store/packages` | `?storeRoot=` optional | Plain text `Packages` index |
| GET | `/store/installed` | `?udid=` | `{ "packages": ["pkg", ...] }` |
| POST | `/store/sync` | `{ "udid", "storeRoot", "syncMode": "file" }` | `{ "ok", "detail" }` |
| POST | `/store/install` | `{ "udid", "package", "storeRoot" }` | `{ "ok", "detail" }` |
| POST | `/store/publish` | `{ "storeRoot", "publishRoot" }` | `{ "publishRoot" }` |

Store operations use SSH (`--normal-ssh`) and auto iproxy.

---

## Environment

| Variable | Default | Purpose |
|----------|---------|---------|
| `PURPLEPOIS0N_BIN` | `build/bin/purplepois0n` | CLI path |
| `PURPLEPOIS0N_AGENT_PORT` | `7749` | Listen port |
| `PURPLEPOIS0N_STORE_ROOT` | `./store` | Host repo |
| `PURPLEPOIS0N_DEVICE_UDID` | — | Default UDID |
| `PURPLEPOIS0N_IPSW` | — | Planner ramdisk auto-resolve |

---

## Example: curl automation

```bash
make agent &
curl -s http://127.0.0.1:7749/health | jq .
curl -s 'http://127.0.0.1:7749/device/plan?udid=YOUR_UDID' | jq .
curl -N -X POST http://127.0.0.1:7749/jailbreak \
  -H 'Content-Type: application/json' \
  -d '{"auto":true,"execute":true,"udid":"YOUR_UDID"}'
```

---

## Related

- [MVP.md](MVP.md) — product journeys and exit criteria
- [DOCTOR.md](DOCTOR.md) — step protocol and GUI launchers
- [STORE_ECOSYSTEM.md](STORE_ECOSYSTEM.md) — store staging and wizard step 5
