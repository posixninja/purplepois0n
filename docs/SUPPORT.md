# Gen 0 support matrix (greenpois0n / absinthe)

purplepois0n is a **research framework**, not a shipping replacement for Chronic Dev Team tools. This table states what historical tools did versus what this repository implements today. For exploit integration boundaries, see [LINEAGE.md](LINEAGE.md) and [book/DEPTH.md](book/DEPTH.md).

**Out of scope in-tree:** limera1n, SHAtter, absinthe payloads, weaponized backup generation, iTunes backup restore to stage exploits, and any complete untether install.

## Capability matrix

| Capability | greenpois0n era | absinthe era | purplepois0n status |
|------------|-----------------|--------------|---------------------|
| DFU USB detect / open | yes | — | **Implemented** (`DeviceManager`, `DFUDevice`) |
| DFU memory R/W, irecv commands | yes | — | **Implemented** (`DFUDevice`, 32-bit USB address split via `IRecvUtil`) |
| DFU bootrom exploit (limera1n / SHAtter) | yes | — | **NOT** — transport only; see `Gen0Workflow` messages |
| Recovery USB detect / open | yes | partial | **Implemented** (`RecoveryDevice`, ECID required) |
| Recovery iBoot commands / ramdisk | yes | — | **Partial** — API + in-memory builder + chain probe; boot **NOT** |
| Recovery ECID in CLI / jailbreak path | yes | — | **Implemented** — Recovery enumeration + `getRecoveryEcid()` in `--gen0` |
| irecv open retry (DFU/Recovery) | — | — | **Implemented** (`IRecvUtil`, 10× / 1s) |
| DFU upload progress (`IRECV_PROGRESS`) | yes | — | **Implemented** (API: `IRecvProgressSubscription`, `DFUDevice::sendFile`) |
| CLI ECID/CPID listing (`-l`) | — | — | **Implemented** (hex ECID, CPID from serial when available) |
| Normal mode lockdown / device info | secondary | yes | **Implemented** (`MobileDevice`) |
| Installed app enumeration | — | yes | **Implemented** (`MobileDevice::getInstalledApplications`) |
| AFC upload / download | post-JB research | yes (staging) | **Implemented** (`AFCService`; CLI `--afc-push` / `--afc-pull`) |
| AFC directory listing | — | — | **Implemented** (`AFCService::listDirectory`; CLI `--afc-list`) |
| Offline backup manifest parse | — | yes | **Implemented** (v1 mbdb/plist + v2 Manifest.db; storage v1/v2) |
| Backup domain / file index | — | yes | **Implemented** (`getDomains`, `getFilesByDomain`, `findFile`) |
| Backup file extract (research) | — | yes | **Implemented** (`extractFile`) |
| Backup restore / malicious staging | — | yes | **NOT** — intentional boundary |
| iTunes/mobilebackup2 restore trigger | — | yes | **NOT** |
| Userland / backup-mediated exploit | rare | yes | **NOT** |
| Untether / persistence install | yes | yes | **NOT** |
| One-click jailbreak (shipping tool) | yes | yes | **NOT** — `performJailbreak()` / `--gen0` scaffold only |
| checkm8 (A5–A11, 2019+) | — | — | **NOT in-tree** — external ([ipwndfu](https://github.com/axi0mX/ipwndfu)); Gen 5 docs |
| Primitive taxonomy (Bootrom/Kernel/Codesign/Sandbox/Injection) | — | — | **Implemented** (`include/primitives/`) |
| Primitive registry + built-in probes | — | — | **Implemented** (`PrimitiveRegistry`, 24 built-ins: Gen6 + historical + cross-gen) |
| Gen 6 Dopamine-shaped chain (12 stages + 6 kernel modules) | — | — | **Implemented** — era-trimmed on iOS &lt; 15 |
| Backport roadmap (modern → older eras) | — | — | **Partial** — [BACKPORT_MATRIX.md](BACKPORT_MATRIX.md); Phase 7 core landed |
| Normal-mode iOS version / ProductType in `--gen0` | — | yes | **Implemented** (`ExecutionContext`, `NormalModeProbePrimitive`) |
| Semi-untether / Dopamine exploit delegate (`dlopen`) | — | yes | **Partial** — `ExploitDelegate` + `PURPLEPOIS0N_DOPAMINE_*` |
| JB installer helper spawn | — | yes | **Partial** — `JbHelperDelegate` + `PURPLEPOIS0N_JB_HELPER` |
| Backup summary in `--gen0` chain | — | yes | **Implemented** (`BackupProbePrimitive` when `--analyze-backup` + device) |
| ChainRunner (Detect→Connect→Probe→Report) | — | — | **Implemented** (`ChainRunner`, wired in `Gen0Workflow`) |
| Mutating primitive ops (Patch/Inject/Execute) | — | — | **Gated** — `make plugins` + `-m`/`allowMutation`; no bundled exploit bytes |
| Normal-mode app-count probe | — | yes | **Implemented** (`NormalModeProbePrimitive` in `--gen0`) |
| Chain report export | — | — | **Implemented** (`--report FILE`, JSON from `ChainRunner`) |
| Offline Mach-O analysis | — | yes | **Implemented** (`MachOBinary` → ipswd / ipsw / `MachOParser`; `--analyze-binary`) |
| Offline dyld cache analysis | — | yes | **Implemented** (`DyldSharedCache` → ipswd / ipsw / `DyldCacheParser`; `--analyze-dyldcache`) |
| Mach-O / dyld JSON export | — | internal | **Implemented** (`--analyze-json`; see [BOOGERAIDS.md](BOOGERAIDS.md)) |
| Encrypted backup decrypt | — | yes | **NOT** — `isEncrypted` detected only; no keybag decrypt |
| TSS / SHSH signing probe | — | yes | **Partial** — `TssSigningProbePrimitive`; idevicerestore / futurerestore / ipsw delegates |
| futurerestore unsigned restore | — | yes | **Partial** — argv builder + env; full restore mutation-gated |
| Host Mach-O / IPA ad-hoc sign | yalu / Fugu15 sideload | yes | **Partial** — `CodesignDelegate`, `--sign-macho` / `--sign-ipa`; needs `make external-ipsw` |
| IPA install over USB (instproxy) | yalu / checkra1n sideload | yes | **Partial** — `SideloadPrimitive`, `--install-ipa`; mutation needs `make plugins` |
| On-device trust cache delegate | Dopamine `jbctl` | — | **Partial** — `TrustCacheDelegate`, `--trustcache-add`; host jbctl or ramdisk SSH |
| Post-jb sign→install→trustcache | yalu / Dopamine sideload | — | **Partial** — `--post-jb-pipeline` (make plugins) |
| futurerestore restore spawn | futurerestore | — | **Partial** — `--futurerestore-restore --i-understand-restore` (make plugins) |
| In-memory HFS+ ramdisk builder | anthrax `template.dmg` | — | **Done** — `HfsPlusWriter`, `--build-ramdisk`, ipsw-validated catalog |
| IPSW RestoreRamDisk mutate + IM4P | greenpois0n staging | — | **Done** — stock extract + overlay merge, `--ramdisk-from-ipsw` |
| Recovery iBSS→iBEC→rdsk chain | idevicerestore FSM | — | **Done** — `RecoveryBootChainPrimitive`, `--recovery-chain` / `--recovery-execute`, optional `go` |
| Live ramdisk upload / exec | jailbreak/custom rdsk only | — | **Partial** — TCP/SSH client + `--ramdisk-add`; user-supplied device agent |

## CLI surfaces (Gen 0)

| Flag | Purpose |
|------|---------|
| `--gen0` | Run Generation 0 scaffold: detect mode, connect where possible, log honest gaps |
| `--analyze-backup PATH` | Offline `MobileBackup` report (domains, counts, metadata); no restore |
| `--analyze-crash PATH` | Offline crash-log ASLR slide table (absinthe-era research; no staging) |
| `--analyze-binary PATH` | Offline Mach-O report; `--arch arm32\|arm64` for fat binaries |
| `--analyze-dyldcache PATH` | Offline dyld shared cache catalog (images, mappings) |
| `--analyze-json FILE` | With analyze-binary/dyldcache: write ipswd/ipsw or internal JSON payload |
| (boogeraids) | See [BOOGERAIDS.md](BOOGERAIDS.md) — **ipswd** (`:3993`) preferred; `external/ipsw/ipsw` fallback |
| `--gen0 --analyze-backup PATH` | Same backup analysis from Normal-mode `--gen0` workflow (via `BackupProbePrimitive`) |
| `--afc-list REMOTE` | List AFC directory (requires `-d UDID`, Normal mode) |
| `--afc-push LOCAL REMOTE` | Upload file via AFC |
| `--afc-pull REMOTE LOCAL` | Download file via AFC |
| `--tss-check` | TSS tool probe + Apple signing check (`ipsw download tss --signed`; needs `-d` Normal) |
| `--ipsw PATH` | Target IPSW for `--gen0` TSS / futurerestore probes |
| `--apticket PATH` | Saved SHSH for futurerestore path |
| `--latest-sep` / `--latest-baseband` / `--no-baseband` | futurerestore SEP/baseband strategy (with `--gen0`) |
| `-j` / default | Jailbreak scaffold: DFU runs probe chain only; other modes use Gen0 workflow |
| `-m` / `--checkm8` | DFU bootrom exploit via external gaster/ipwndfu (after probe) |
| `-l` | List devices (Normal / Recovery / DFU) with hex ECID and CPID when available |
| `-d UDID` | Target normal-mode device |
| `--report FILE` | Write `ChainRunner` JSON report (use with `--gen0`, `-j`, or `-m`) |
| `--sign-macho` / `--sign-app` / `--sign-ipa` | Host codesign via ipsw (`--sign-id`, `--ent`, `--ad-hoc`, `--output`) |
| `--install-ipa PATH` | Normal-mode IPA install (`-d UDID`; execute gated) |
| `--trustcache-add PATH` | Post-jailbreak trust cache probe (`PURPLEPOIS0N_JBCTL`) |
| `--build-ramdisk PATH` | In-memory HFS+ volume from `--ramdisk-overlay` (no hdiutil) |
| `--ramdisk-from-ipsw` | Stock RestoreRamDisk → lzss IM4P (`--ipsw`, optional overlay) |
| `--recovery-chain` / `--recovery-execute` | Multi-stage Recovery boot probe / execute (`--gen0` or standalone) |
| `--ramdisk-probe` / `--ramdisk-exec` / `--ramdisk-push` / `--ramdisk-pull` | SSH to **custom** booted ramdisk (overlay must inject sshd; stock rdsk has none) |

Deep dive: [book/deep/sideload-codesign.md](book/deep/sideload-codesign.md), [book/deep/recovery-ramdisk.md](book/deep/recovery-ramdisk.md).

Programmatic entry: [`Gen0Workflow`](../src/Gen0Workflow.h) (`runGen0Jailbreak`, `analyzeBackup`, `analyzeBinary`, `analyzeDyldCache`); primitive framework in [`include/primitives/`](../include/primitives/Primitives.h).

## What to use instead (external)

| Historical need | In-tree today | External reference (educational) |
|-----------------|---------------|----------------------------------|
| A4 bootrom (limera1n) | `DFUDevice` I/O only | geohot limera1n / contemporary redsn0n bundles |
| Old-BR 3GS / iPod 2G (24kpwn) | `DFUDevice` I/O only | redsn0w / Chronic Dev IMG3 bundles (`gen0-24kpwn` stub) |
| A5–A11 bootrom (checkm8) | `DFUDevice` I/O only | [ipwndfu](https://github.com/axi0mX/ipwndfu), checkra1n |
| Absinthe backup delivery | `MobileBackup` parse only | Historical write-ups; **no reproduction here** |

## Related docs

- [LINEAGE.md](LINEAGE.md) — predecessors and framework role
- [GENERATIONS.md](GENERATIONS.md#generation-0-chronic-dev-era-predecessors) — Generation 0 detail
- [book/00-chronic-dev-greenpois0n.md](book/00-chronic-dev-greenpois0n.md) — greenpois0n L5
- [book/01-chronic-dev-absinthe.md](book/01-chronic-dev-absinthe.md) — absinthe L5
- [book/deep/dfu-recovery.md](book/deep/dfu-recovery.md) — DFU / Recovery classes
- [book/deep/normal-mode-afc-backup.md](book/deep/normal-mode-afc-backup.md) — parse vs restore boundary
- [book/deep/binary-parsers.md](book/deep/binary-parsers.md) — Mach-O / dyld cache / ipswd
- [BOOGERAIDS.md](BOOGERAIDS.md) — ipswd + `--analyze-json` handoff

## Manual smoke checks (no device required for compile)

```bash
make release          # native host arch; requires libimobiledevice + libirecovery (see README)
make ARCH=x86_64 release   # Intel slice (when deps available)
make ARCH=arm64 release    # Apple Silicon slice
make test-fixtures    # backup fixtures + /bin/ls Mach-O (offline)
./build/bin/purplepois0n -l
./build/bin/purplepois0n --gen0 --report /tmp/pp-report.json
```

**ipswd (optional, preferred for binary analysis):**

```bash
make external-ipswd
./external/ipsw/ipswd start
curl -sS http://127.0.0.1:3993/v1/_ping    # OK
./build/bin/purplepois0n --analyze-binary /bin/ls   # Backend: ipswd
```

With a connected device:

| Mode | Command | Expected |
|------|---------|----------|
| DFU | `./build/bin/purplepois0n --gen0 --report /tmp/dfu.json` | `[Detect]`/`[Probe]` stages; Bootrom CPID line; no external tool unless `-m` |
| Recovery | `./build/bin/purplepois0n --gen0` | Recovery ECID logged; `[Probe]` offline codesign probe |
| Normal (trusted) | `./build/bin/purplepois0n --gen0 --report /tmp/n.json` | `[Normal] installed apps: N`; Injection/AFC probe |
