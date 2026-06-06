# Legacy repository index

Compact index of local mirrors under [`legacy/`](../../legacy/). **Study priority:** P0 = read first for purplepois0n Gen-0 work; P1 = high value; P2 = cross-generation reference; P3 = catalog only.

**Snapshot:** 2026-06-03 · **Bulk mirrors:** 111 repos (Chimera13 **not cloned** — HTTP 451) · **Gen 6 selective:** 16 repos in `legacy/modern-era/`

**Deep dives:** P0/P1 rows below · **Gen 6 synthesis:** [MODERN_ERA_LEARNINGS.md](MODERN_ERA_LEARNINGS.md)

---

## modern-era/ — Generation 6 (selective clones)

Refresh: [`legacy/clone-modern-era.sh`](../../legacy/clone-modern-era.sh)

| Repo | Lang | ~Purpose | Maps to purplepois0n | Priority |
|------|------|----------|----------------------|----------|
| **Dopamine** | ObjC/Swift/C | Rootless JB app + BaseBin + exploit frameworks | Phase 6 host/device boundary; **do not port** exploits | **P0** |
| **kfd-opa334** | C | libkfd fork for Dopamine | Vocabulary for `KernelCapabilityProbePrimitive` | **P0** |
| **kfd-felix-pb** | C | Original kfd + PUAF write-ups | Book / puaf-kfd-era citations | **P0** |
| **XPF** | C | Kernel patchfinder (Choma) | Parallel to offline kernelcache analysis | **P0** |
| **weightBufs** | C | ANE kernel exploit | Picker module study | **P0** |
| **multicast_bytecopy** | C | Multicast CoW kernel exploit | Picker module study | **P0** |
| **darksword-kexploit** | ObjC | Kernel port (ITW class) | 2.5b arm64 17–18 range context | **P0** |
| **TrollStore** | ObjC | Permasigned IPA install | Phase 6.7 delegate / user docs | **P1** |
| **libroot** | C | Rootless path prefixes | AFC / path docs post-JB | **P1** |
| **libkrw** | C | Kernel R/W API + plugins | Dopamine `libkrw-provider` pattern | **P1** |
| **libgrabkernel2** | C | On-device kernelcache fetch | Contrasts with host IPSW path | **P1** |
| **Procursus** | mixed | Rootless bootstrap | Out of repo scope | **P1** |
| **ellekit** | Swift/C | Tweak injection | Bootstrap stack context | **P1** |
| **Fugu15** | mixed | Dopamine 1.x lineage | arm64e / badRecovery era | **P2** |
| **multicast_bytecopy_A9** | C | A9 fork | Device-specific picker | **P2** |
| **DarkSword-Analysis** | docs | ITW chain notes | Threat-intel cross-check | **P2** |

---

## P0 / P1 detail

### Chronic-Dev

| Repo | Lang | ~Purpose | Maps to purplepois0n | Priority |
|------|------|----------|----------------------|----------|
| **syringe** | C | DFU/Recovery USB library + inject API | `DFUDevice`, `RecoveryDevice`, future progress callbacks | **P0** |
| **libirecovery** | C | irecv USB client (fork → libimobiledevice) | Same API purplepois0n links | **P0** |
| **gp2** | C | greenpois0n RC build orchestration | Build/plugin ordering reference | **P0** |
| **gprc5** | C/ObjC | RC5 GUI + embedded syringe | GUI/workflow reference | **P0** |
| **greenpois0n** | meta | Submodule meta-repo | **Empty submodules** — use syringe standalone | **P0** (caveat) |
| **absinthe-2.0** | C/C++ | iOS 5.1.1 backup-mediated JB + Mach-O/mbdb | `MobileBackup`, `MachOParser`, `MobileDevice` | **P0** |
| **absinthe** | C | A5 absinthe CLI (earlier) | Normal-mode workflow ancestor | **P1** |
| **apparition** | C | mobilebackup2 / mbdb research | `MobileBackup` mbdb gap | **P1** |
| **idevicerestore** | C | Full restore FSM (DFU→iBoot→ASR) | irecv retry/progress patterns | **P1** |
| **libchronic** | C | file/debug/endian utils | Logger/path helpers (optional) | **P1** |
| **anthrax** | C/shell | Ramdisk build | Do not port; understand ramdisk role | **P1** (study) |
| **medicine** | mixed | Installers/assets | Packaging reference | **P2** |
| **doctors** | C | CLI (`injectpois0n`) | CLI UX patterns | **P1** |
| **poison-jb** | mixed | JB bundle fragments | Historical packaging | **P2** |
| **cyanide** | mixed | Payload packaging | Payload pipeline shape | **P2** |
| **irecovery** | C | irecv CLI tool | CLI command parity | **P1** |

### posixninja

| Repo | Lang | ~Purpose | Maps to purplepois0n | Priority |
|------|------|----------|----------------------|----------|
| **libirecovery-2.0** | C | Author irecv fork | Diff vs Chronic-Dev/libirecovery | **P0** |
| **libchronic** | C | Utility library | Shared with absinthe | **P1** |
| **libmacho** | C | Mach-O parser | `MachOParser` coverage audit | **P1** |
| **syringe** | C | Author syringe fork | Diff Chronic-Dev/syringe | **P1** |
| **xpwn** | C | IPSW/img3/iboot tools | Future IMG3 parser | **P1** |
| **spirit-linux** | C | Normal-mode JB host (iOS 3 era) | Pre-absinthe normal patterns | **P2** |

### OpenJailbreak (P0/P1 only)

| Repo | Lang | ~Purpose | Maps to purplepois0n | Priority |
|------|------|----------|----------------------|----------|
| **libmbdb** | C | Standalone mbdb parser | `MobileBackup` Phase 2 | **P0** |
| **libirecovery** | C | Maintained irecv | System dependency | **P0** |
| **libimobiledevice** | C | Lockdown/usbmux stack | `MobileDevice`, `AFCService` | **P0** |
| **ipwndfu** | Python | checkm8 exploit | `Checkm8` external ref | **P1** |
| **libdyldcache** | C | Dyld shared cache | `DyldCacheParser` | **P1** |
| **libmacho** | C | Mach-O library | `MachOParser` | **P1** |
| **absinthe-2.0** | C/C++ | Mirror of Chronic-Dev | Diff / backup | **P1** |
| **greenpois0n** | meta | Mirror | Diff only | **P2** |
| **Chimera13** | — | A12 JB | **Not cloneable (451)** | n/a |

---

## Chronic-Dev — full catalog (P2/P3)

| Repo | Lang | ~Purpose | Maps to purplepois0n | Priority |
|------|------|----------|----------------------|----------|
| Bootrom-Dumper | C | Bootrom dump research | Bootrom education | P3 |
| ambrosia | mixed | Asset/tooling | — | P3 |
| animate | mixed | Boot animation | — | P3 |
| arsenic | mixed | Tooling | — | P3 |
| cdevreporter | mixed | Crash reporter | — | P3 |
| curl | C | Vendored dep | — | P3 |
| dioxin | mixed | Tooling | — | P3 |
| feedface | mixed | UI/assets | — | P3 |
| genpass | C | Key generation | — | P3 |
| gnutls | C | Vendored TLS | — | P3 |
| iOS-CDevReporter | ObjC | iOS reporter | — | P3 |
| ideviceactivate | C | Activation | — | P3 |
| libcnary | C | CNary plist | — | P3 |
| libgcrypt | C | Vendored crypto | — | P3 |
| libgpg-error | C | Vendored dep | — | P3 |
| libplist | C++ | Plist library | libplist dep | P2 |
| libtasn1 | C | Vendored ASN.1 | — | P3 |
| libusb | C | Vendored USB | — | P3 |
| libxml2 | C | Vendored XML | — | P3 |
| openssl | C | Vendored TLS | — | P3 |

---

## OpenJailbreak — full catalog (P2/P3)

| Repo | Lang | ~Purpose | Maps to purplepois0n | Priority |
|------|------|----------|----------------------|----------|
| Fugu | mixed | Research tool | — | P3 |
| JailbreakMe-1.0 | mixed | Web JB | — | P3 |
| SockPuppet | mixed | CVE exploit | Do not port | P3 |
| Undecimus | Swift/C | iOS 11–12 JB | Gen 4 reference | P2 |
| absinthe | C | Mirror | — | P2 |
| absinthe-1 | C | Mirror | — | P3 |
| acorn | mixed | Tooling | — | P3 |
| async_wake_ios | C | Exploit research | Do not port | P3 |
| atrix | mixed | Tooling | — | P3 |
| community | meta | Org meta | — | P3 |
| evasi0n6 | C++ | iOS 6 JB GUI | Normal-mode GUI patterns | P2 |
| feedface | mixed | Assets | — | P3 |
| iBootRE | C | iBoot reverse | — | P3 |
| iOSRE | mixed | RE notes | — | P3 |
| idevicerestore | C | Restore tool | Same as Chronic-Dev | P2 |
| libcrippy | C | Binary utils | — | P3 |
| libdmg | C | DMG parse | — | P3 |
| libhfsplus | C | HFS+ parse | — | P3 |
| libimg3 | C | IMG3 format | IMG3 future work | P2 |
| libipsw | C | IPSW handling | — | P2 |
| liboffsetfinder64 | C | Offset finding | — | P3 |
| libplist | C++ | Plist | dep | P2 |
| libqmi | C | Qualcomm | — | P3 |
| libtss | C | TSS/SHSH | Restore signing education | P2 |
| libusbmuxd | C | usbmux | `MobileDevice` dep | P2 |
| n1ghtshade | mixed | Tooling | — | P3 |
| oob_timstamp_ppl | C | Exploit | Do not port | P3 |
| p0sixspwn | mixed | A4 JB | Gen 0 related | P2 |
| pris0nbarake | mixed | Tooling | — | P3 |
| purplesn0w | mixed | ultrasn0w era | — | P3 |
| racism | mixed | Tooling | — | P3 |
| spirit | mixed | iOS 3 JB | — | P3 |
| star | mixed | Tooling | — | P3 |
| sudochop | mixed | Tooling | — | P3 |
| voucher_swap | C | Exploit | Do not port | P3 |
| yalu | C | iOS 10 JB | Gen 3 reference | P2 |
| yalu102 | C | iOS 10.2 | Gen 3 reference | P2 |
| yellowsn0w | mixed | Baseband | — | P3 |

---

## posixninja — full catalog (P2/P3)

| Repo | Lang | ~Purpose | Maps to purplepois0n | Priority |
|------|------|----------|----------------------|----------|
| BBTool | C | Baseband tool | — | P3 |
| DBLTool | C | DBL tool | — | P3 |
| DLOADTool | C | Download mode | — | P3 |
| Tasks-Explorer | mixed | RE tool | — | P3 |
| anthrax | mixed | Ramdisk | — | P3 |
| arcan | C | Display server | — | P3 |
| cyanide | mixed | Payloads | — | P3 |
| genpass | C | Keys | — | P3 |
| gr-specest | mixed | SDR | — | P3 |
| hackrf | C | SDR | — | P3 |
| hxxpsin | mixed | Tooling | — | P3 |
| iDict | mixed | Tooling | — | P3 |
| iOSUSBEnum | C | USB enum | USB education | P2 |
| ideviceactivate | C | Activation | — | P3 |
| iphonelinux | mixed | Linux port | — | P3 |
| jboot | C | Boot research | — | P3 |
| kextstat_aslr | C | KASLR research | — | P3 |
| libcnary | C | CNary | — | P3 |
| libimobiledevice | C | libimobiledevice | dep | P2 |
| lldb | C++ | Debugger fork | — | P3 |
| pppoccl | mixed | Tooling | — | P3 |
| senseye | mixed | Tooling | — | P3 |
| sudochop | mixed | Tooling | — | P3 |

---

## Quick lookup: purplepois0n component → legacy sources

| purplepois0n | Read first (P0) |
|--------------|-----------------|
| `DFUDevice` / `RecoveryDevice` / `IRecvUtil` | `Chronic-Dev/syringe`, `Chronic-Dev/libirecovery`, `idevicerestore/src/dfu.c` |
| `DeviceManager` | `syringe/libirecovery.c` mode constants; compare `IRecvUtil` retry to `dfu.c` |
| `include/primitives/` / `Gen0Workflow` | `gp2/Makefile`, `doctors/cli/injectpois0n` (shape only); `absinthe.c` CLI flow |
| `MobileBackup` | `absinthe-2.0/src/mbdb.c`, `OpenJailbreak/libmbdb`, `apparition/src/backup.c` |
| `MbdbParser` | `OpenJailbreak/libmbdb/src/mbdb*.c` (format reference only) |
| `ManifestDbParser` | Community Manifest.db schema (SQLite `Files` table) |
| `KeyedArchiverPlist` | Community NSKeyedArchiver `$objects` patterns |
| `MachOParser` | `absinthe-2.0/src/macho*.c`, `posixninja/libmacho` |
| `DyldCacheParser` | `absinthe-2.0/src/dyldcache.c`, `OpenJailbreak/libdyldcache` |
| `MobileDevice` / `AFCService` | `absinthe-2.0/src/lockdown.c`, `afc.c` |
| `Checkm8` | `OpenJailbreak/ipwndfu` (external only) |

---

## See also

- [LEARNINGS.md](LEARNINGS.md) — architecture synthesis
- [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md) — phased port plan
- [PHASE_STATUS.md](PHASE_STATUS.md) — living phase rollup
- [COMPARISON_MATRIX.md](COMPARISON_MATRIX.md) — capability grid
