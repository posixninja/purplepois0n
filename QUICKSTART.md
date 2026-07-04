# Purplepois0n Quick Start

**MVP goal:** Scan the device → pick a jailbreak strategy → run it (CLI, agent, or web wizard).

Full scope and status: **[docs/MVP.md](docs/MVP.md)**.

---

## 1. Build

```bash
cd /path/to/purplepois0n
make release          # host binary
make plugins          # mutating primitives (required for live jailbreak)
make kpf              # default boot module (DFU usb-loader path)
```

Verify:

```bash
./build/bin/purplepois0n --capabilities
# plugins: true, kpf.built: true (after steps above)
```

---

## 2. Web UI (recommended MVP demo)

Terminal 1 — host agent:

```bash
export PURPLEPOIS0N_IPSW=/path/to/firmware.ipsw   # optional; DFU/recovery ramdisk
make agent
```

Terminal 2 — web app:

```bash
make web-dev
# open URL printed by Vite (usually http://localhost:5173)
```

Flow: **Jailbreak wizard** → connect device → step **Jailbreak** (5th dot) shows **Recommended path** from `/device/plan` → consent → **Jailbreak** or **Verify jailbreak** → step **All set** (6th dot) store sync.

---

## 3. CLI — scan and plan (no mutation)

```bash
./build/bin/purplepois0n --list
./build/bin/purplepois0n device plan              # JSON: device + strategy + blockers
./build/bin/purplepois0n jailbreak                # doctor probe only (plan + steps, no jailbreak step)
# legacy: ./build/bin/purplepois0n --doctor-run --normal-ssh
```

---

## 4. CLI — execute (device required)

```bash
export PURPLEPOIS0N_IPSW=/path/to/firmware.ipsw     # if plan needs ramdisk pack

./build/bin/purplepois0n jailbreak --execute
# includes --normal-ssh for Normal-mode /var/jb detection
```

DFU checkm8 path (explicit):

```bash
make plugins kpf
./build/bin/purplepois0n --dfu-jailbreak --i-understand-jailbreak
```

---

## 5. Already jailbroken → store only

```bash
export PURPLEPOIS0N_NORMAL_SSH=1   # or use make agent
./build/bin/purplepois0n device plan -d UDID
./build/bin/purplepois0n jailbreak --execute -d UDID   # verify /var/jb
make seed-store
./build/bin/purplepois0n store sync -d UDID
./build/bin/purplepois0n store install purplepois0n-zebra -d UDID
```

See [docs/STORE_ECOSYSTEM.md](docs/STORE_ECOSYSTEM.md).

---

## 6. Delegate-first (palera1n / Dopamine)

When the planner selects `normal-external-delegate`:

```bash
make plugins
export PURPLEPOIS0N_NORMAL_SSH=1
./build/bin/purplepois0n external-jailbreak --i-understand-jailbreak -d UDID
```

Gen 6: jailbreak with **Dopamine on device first**, enable SSH, then use **Already jailbroken** in the web wizard or `--already-jailbroken` above.

---

## Environment cheatsheet

| Variable | Use |
|----------|-----|
| `PURPLEPOIS0N_IPSW` | Auto-pack ramdisk from firmware |
| `PURPLEPOIS0N_RAMDISK` | Prebuilt `.dmg` instead of IPSW |
| `PURPLEPOIS0N_BOOT_MODULE` | Boot module (default: built KPF) |
| `PURPLEPOIS0N_NORMAL_SSH` | SSH for store / rootless on Normal mode |
| `PURPLEPOIS0N_STORE_ROOT` | Host package repo (default `store/`) |

---

## Smoke tests (offline)

```bash
make smoke-mvp
make smoke-mvp-strict   # optional: + capabilities, fixtures, rootless
```

Details: [docs/validation/mvp-smoke.md](docs/validation/mvp-smoke.md).

---

## Read next

1. **[docs/MVP.md](docs/MVP.md)** — MVP architecture, API, honest status
2. **[docs/STORE_ECOSYSTEM.md](docs/STORE_ECOSYSTEM.md)** — package store
3. **[docs/book/deep/recovery-ramdisk.md](docs/book/deep/recovery-ramdisk.md)** — ramdisk + boot lanes
4. **[docs/SUPPORT.md](docs/SUPPORT.md)** — capability matrix vs historical tools

Research / legacy depth: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/FINAL_STATUS.md](docs/FINAL_STATUS.md) (historical implementation notes).
