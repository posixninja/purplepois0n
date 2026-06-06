# Legacy comparison matrix

Capability grid: **greenpois0n era** (Chronic-Dev DFU tools) vs **absinthe era** (backup-mediated) vs **purplepois0n today**.

Legend: **Yes** = implemented in shipping historical tool / in-tree today · **Partial** = scaffold or transport only · **No** = absent · **N/A** = not applicable to that era · **Ext** = delegated to external tool · **NOT** = intentionally out of scope

| Capability | greenpois0n (gp2/gprc5/syringe) | absinthe / absinthe-2.0 | purplepois0n |
|------------|-----------------------------------|-------------------------|--------------|
| **DFU USB detect / open** | Yes | N/A | **Yes** (`DeviceManager`, `DFUDevice`) |
| **DFU memory R/W** | Yes | N/A | **Yes** |
| **DFU bootrom exploit (limera1n/SHAtter)** | Yes | N/A | **NOT** |
| **checkm8 (A5–A11)** | N/A | N/A | **Ext** (`Checkm8` → gaster/ipwndfu) |
| **Recovery USB detect / open** | Yes | Partial | **Yes** (`RecoveryDevice`) |
| **Recovery ECID-scoped open** | Yes | N/A | **Yes** (`getRecoveryEcid`, enumeration) |
| **irecv open retry** | Partial | N/A | **Yes** (`IRecvUtil`) |
| **CLI ECID/CPID listing** | Partial | N/A | **Yes** (`-l`) |
| **iBoot commands / ramdisk load** | Yes | N/A | **Partial** (API only) |
| **IMG3 / iBSS upload** | Yes | N/A | **No** |
| **Normal mode lockdown** | Secondary | Yes | **Yes** (`MobileDevice`) |
| **Installed app enumeration** | No | Yes | **Yes** |
| **AFC upload / download** | Post-JB | Yes | **Yes** (`AFCService`; CLI `--afc-push` / `--afc-pull`) |
| **AFC directory listing** | Post-JB | Yes | **Yes** (`--afc-list`) |
| **Offline backup parse (v1 index)** | No | Yes (mbdb/plist) | **Yes** |
| **Offline backup parse (v2 index)** | No | — | **Yes** (`Manifest.db`) |
| **Backup storage v1/v2 paths** | No | Yes | **Yes** (`Status.plist` + flat/sharded resolve) |
| **Backup domain / file index** | No | Yes | **Yes** |
| **Backup file extract** | No | Yes | **Yes** |
| **Backup restore / staging** | No | Yes | **NOT** |
| **mobilebackup2 live client** | No | Yes | **No** |
| **Mach-O parse (host)** | Limited | Yes | **Yes** — `MachOBinary` → ipswd / ipsw / `MachOParser`; `--arch` for fat |
| **Mach-O JSON export (boogeraids)** | No | Internal | **Yes** (`--analyze-json`, ipswd-first) |
| **PUAF / kfd kernel research (host)** | No | Internal (kfd) | **No** — study external kfd; offline IPSW parse only |
| **Dyld shared cache parse** | No | Yes | **Yes** — `DyldSharedCache` → ipswd / ipsw / `DyldCacheParser` |
| **Encrypted backup decrypt** | No | Yes | **No** (deferred; parse metadata only) |
| **Crash log → ASLR slide** | No | Yes | **No** |
| **Userland / backup-mediated exploit** | No | Yes | **NOT** |
| **Untether install** | Yes | Yes | **NOT** |
| **One-click GUI jailbreak** | Yes | Yes | **No** (CLI scaffold) |
| **Gen0 honest gap logging** | No | No | **Yes** (`Gen0Workflow`) |
| **Primitive taxonomy + ChainRunner** | No | No | **Yes** (`include/primitives/`, 7 built-in probes) |
| **Gen 6 host probes (ipswd/kernel/sandbox gaps)** | No | No | **Yes** (Phase 6) |
| **Normal app-count probe** | No | Yes | **Yes** (`NormalModeProbePrimitive`) |
| **Chain report export** | No | No | **Yes** (`--report FILE`) |
| **Progress callbacks (USB upload)** | Yes | Partial | **Yes** (`IRecvProgressSubscription`, optional `DFUDevice` callback) |
| **iTunes/Finder interference handling** | Partial | Yes (`iTunesKiller`) | **No** |
| **TSS / SHSH / signed restore** | via redsn0n ecosystem | Partial | **Partial** (probe + futurerestore delegate) |
| **Device mode auto-detect order** | DFU→Recovery→Normal | Normal-first | **DFU→Recovery→Normal** |

---

## Mode entry comparison

| Mode | greenpois0n primary? | absinthe primary? | purplepois0n support |
|------|---------------------|-------------------|---------------------|
| DFU | **Primary** | No | Transport + checkm8 ext |
| Recovery | Secondary | Rare | Transport partial |
| Normal | Rare | **Primary** | Lockdown + backup parse |

---

## Parser comparison

| Format | greenpois0n | absinthe-2.0 | purplepois0n |
|--------|-------------|---------------|--------------|
| Manifest.plist | — | Yes | **Yes** |
| Manifest.mbdb | — | Yes (`mbdb.c`) | **Yes** |
| Mach-O load commands | payloads only | Full (`macho_*.c`) | **Yes** — ipswd/ipsw JSON or in-tree `MachOParser` |
| dyld_shared_cache | — | Yes (`dyldcache.c`) | **Yes** — ipswd/ipsw JSON or in-tree `DyldCacheParser` |
| IMG3 / IM4P | syringe payloads | Limited | **No** |
| IPsw / firmware bundles | cyanide/gp2 | — | **No** |

---

## CLI / workflow comparison

| Surface | greenpois0n | absinthe | purplepois0n |
|---------|-------------|----------|--------------|
| One-shot jailbreak | GUI / injectpois0n | GUI / CLI | `-j` / `--gen0` scaffold |
| List devices | Partial | Partial | `-l` |
| Analyze backup offline | — | Internal | `--analyze-backup PATH` |
| Analyze Mach-O / dyld cache offline | — | Internal | `--analyze-binary`, `--analyze-dyldcache`, `--analyze-json` |
| Gen0 + backup hook | — | Internal | `--gen0 --analyze-backup PATH` |
| Explicit checkm8 | — | — | `-m` / `--checkm8` |
| Chain report JSON | — | — | `--report FILE` |
| Honest capability doc | — | — | `docs/SUPPORT.md` |

---

## Source anchors (study paths)

| Tool | Representative legacy paths |
|------|----------------------------|
| greenpois0n | `legacy/Chronic-Dev/syringe/`, `gp2/`, `gprc5/` |
| absinthe | `legacy/Chronic-Dev/absinthe-2.0/src/`, `gui/` |
| purplepois0n | `src/Gen0Workflow.cpp`, `src/MobileBackup.cpp`, `src/MachOBinary.cpp`, `src/IpswdClient.cpp`, `include/primitives/` |

---

## Maintenance

Update this matrix when:

- Hardware smoke: DFU/Recovery/checkm8 with physical device (documented in SUPPORT.md)
- SUPPORT.md capability rows change
- Binary analysis backend changes (ipswd / ipsw / internal)
- Backport tasks land — sync [BACKPORT_MATRIX.md](../BACKPORT_MATRIX.md) §5–§6

See [PHASE_STATUS.md](PHASE_STATUS.md) for current phase rollup.

See [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md) for phase acceptance criteria.
