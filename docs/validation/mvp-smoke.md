# MVP smoke validation

Run these before a demo or release tag. All offline tests expect `make release` unless noted.

## Offline CI

```bash
make release
make smoke-mvp
```

Extended (capabilities + rootless layout + parser fixtures + MVP gap invariants):

```bash
make smoke-mvp-strict
```

Optional live device (set `PURPLEPOIS0N_DEVICE_UDID` first):

```bash
make smoke-hardware-validation
```

| Target | Validates |
|--------|-----------|
| `smoke-mvp` | **All rows below** + `web-build` |
| `smoke-mvp-strict` | `smoke-mvp` + `smoke-capabilities` + `smoke-rootless-layout` + `smoke-mvp-gaps` + `smoke-recovery-chain` + `test-fixtures` |
| `smoke-mvp-gaps` | Offline gap A/B/C source + pongo-boot report |
| `smoke-recovery-chain` | Recovery planner chain from IPSW + CLI flags |
| `smoke-hardware-validation` | `smoke-mvp-gaps` + DFU wiring + optional live UDID checks |
| `smoke-kpf` | KPF builds; patchfinder test binary |
| `smoke-dfu-jailbreak` | DFU help, guard flags, KPF path |
| `smoke-dpkg-store` | Store init/build/Packages index |
| `smoke-doctor` | `--doctor-run` emits step + complete JSON |
| `smoke-agent` | Health, `/devices`, `/store/packages`, `/doctor` probe (no jailbreak step), `/jailbreak` auto probe |
| `smoke-device-plan` | `--device-plan` + `device plan` subcommand (15s timeout) |
| `smoke-capabilities` | `--capabilities` JSON (`doctor`, `store`, `kpf`) |
| `web-build` | TypeScript + Vite production build |

## Host readiness

```bash
./build/bin/purplepois0n --capabilities
./build/bin/purplepois0n dev --help | head -30
```

Expect `plugins: true` after `make plugins`, and `kpf.built: true` after `make kpf`.

## Hardware (optional CI / pre-demo)

Requires jailbroken device with SSH unless noted.

```bash
export PURPLEPOIS0N_DEVICE_UDID=YOUR_UDID
export PURPLEPOIS0N_NORMAL_SSH=1
make smoke-e2e-delegate    # probe /var/jb + store sync + install smoke package
make smoke-store-device    # store sync + install only
```

`smoke-agent` **fails** (not warns) when `PURPLEPOIS0N_DEVICE_UDID` is set and `/device/plan` errors.

## With device (manual)

### Normal — already jailbroken

1. Dopamine/palera1n + SSH enabled on device.
2. `export PURPLEPOIS0N_NORMAL_SSH=1` (or use `make agent`, which sets it).
3. `./build/bin/purplepois0n device plan -d UDID` → `normal-already-jailbroken`
4. `./build/bin/purplepois0n jailbreak --execute -d UDID` → verifies `/var/jb` (no `make plugins` required).
5. Web wizard **All set** (step 6 of 6): store sync + zebra install.

### Normal — external delegate

1. `./build/bin/purplepois0n device plan -d UDID` → `normal-external-delegate` or `normal-gen6-in-tree`
2. `make plugins` && external helper installed.
3. `./build/bin/purplepois0n jailbreak --execute -d UDID`
4. Store sync is **not** automatic after delegate execute — use web wizard **All set**, `--post-jb-store`, agent `post_jb_store: true`, or `PURPLEPOIS0N_POST_JB_STORE=1` on plan/execute.

### DFU — usbliter8 (A12/A13)

1. Device in DFU on supported CPID.
2. `make plugins`
3. `device plan` → `dfu-usbliter8` (no KPF/ramdisk blockers).
4. `jailbreak --execute` runs bootrom only — no Pongo/usb-loader stage.

### DFU — checkm8 path

1. Device in DFU; `./build/bin/purplepois0n --list` shows DFU entry.
2. `export PURPLEPOIS0N_IPSW=/path/to/ipsw`
3. `make plugins kpf`
4. `./build/bin/purplepois0n device plan` → `dfu-checkm8-usb-loader`, `ramdiskSource: ipsw-pack`, no blockers (if plugins+kpf+libusb).
5. `./build/bin/purplepois0n jailbreak --execute --i-understand-jailbreak`

### Recovery — ramdisk chain

1. Device in Recovery; `PURPLEPOIS0N_IPSW` set; `make plugins`.
2. `device plan` → `recovery-ramdisk-chain`, `canExecute: true`.
3. `jailbreak --execute` sets `recovery.execute` and runs upload chain.

### Web UI

1. `make agent` + `make web-dev`
2. Jailbreak wizard step **Jailbreak** (5th dot) shows plan card after rescan.
3. **Jailbreak** / **Verify jailbreak** streams doctor steps; probe-only does not run jailbreak step.
4. **All set** runs store sync + zebra install.

## Verification matrix (post-gap remediation)

| Scenario | Command | Expected |
|----------|---------|----------|
| Offline CI | `make smoke-mvp-strict` | Green |
| Probe only | `jailbreak` (no `--execute`) | Plan JSON, no jailbreak step |
| Already JB verify | Web Verify or `jailbreak --execute` | `/var/jb` confirmed, **no** store sync |
| Already JB store | Wizard All set | sync + zebra once |
| DFU checkm8 | `device plan` + `jailbreak --execute` + IPSW/KPF/plugins | Bootrom + usb-loader |
| DFU usbliter8 | Same on A12 DFU | Bootrom usbliter8 only, no Pongo |
| Hardware store | `PURPLEPOIS0N_DEVICE_UDID=… make smoke-e2e-delegate` | sync + smoke package |
| Recovery chain | `device plan` in Recovery + `PURPLEPOIS0N_IPSW` | `recoveryChain[]` in plan JSON |
| Fixtures | `make test-fixtures` | Chain report JSON, no crash |

## Hardware sign-off log

| Date | Device / mode | Result | Notes |
|------|---------------|--------|-------|
| 2026-07-04 | Offline CI | Pass | `make smoke-mvp-strict`, `smoke-mvp-gaps`, `smoke-recovery-chain`, `smoke-hardware-validation` (no UDID — live steps skipped) |
| — | Normal already-JB | Pending | Set `PURPLEPOIS0N_DEVICE_UDID` + `make smoke-hardware-validation` |

## Failure triage

| Symptom | Likely fix |
|---------|------------|
| Plan blocker: `make plugins` | `make plugins` && rebuild |
| Plan blocker: boot module | `make kpf` |
| Plan blocker: ramdisk | Set `PURPLEPOIS0N_IPSW` or `PURPLEPOIS0N_RAMDISK` |
| Agent 404 on `/device/plan` | Rebuild binary; restart agent |
| Store sync fails | `PURPLEPOIS0N_NORMAL_SSH=1`, trust device, jbroot present |
| Doctor probe runs jailbreak | Upgrade binary; probe must stop after plan JSON |
