# Backport matrix — modern framework → older generations

This document answers: **what from purplepois0n’s newer work can apply to earlier iOS / jailbreak eras**, and what cannot. It complements the era-vs-era grid in [legacy/COMPARISON_MATRIX.md](legacy/COMPARISON_MATRIX.md) and the generation reference in [GENERATIONS.md](GENERATIONS.md).

**Policy (unchanged):** backport **host I/O, parsers, orchestration, and delegate hooks** — not in-tree exploit blobs, ramdisks, or weaponized backup staging. See [LINEAGE.md](LINEAGE.md) and [ARCHIVES.md](ARCHIVES.md).

---

## Legend

| Symbol | Meaning |
|--------|---------|
| **Done** | Implemented in `src/` today |
| **Partial** | Scaffold, transport, or delegate only |
| **Planned** | Tracked in [INTEGRATION_PLAN.md](legacy/INTEGRATION_PLAN.md) Phase 7 |
| **Study** | Safe to read from `legacy/` mirrors; port patterns only |
| **N/A** | Mitigation or OS layout makes the feature irrelevant |
| **NOT** | Intentionally out of scope |

**Directions:**

- **Universal** — already works on any connected device or offline artifact, regardless of era.
- **Framework backport** — Gen 6 (or modern) *patterns* reused for Gen 0–5 chains.
- **Legacy backport** — Chronic-Dev / OpenJailbreak *host features* not yet in purplepois0n.
- **No backport** — on-device exploit or bypass that is build- and mitigation-specific.

---

## Summary counts

| Bucket | Items | Status |
|--------|-------|--------|
| Universal host + research surfaces | **15** | **Done** (see §1) |
| Framework patterns portable to older gens | **5** | **1 done**, **4 planned** |
| Legacy mirror host features still to port | **10** | **0 done**, **10 planned/study** |
| Gen 6 exploit primitives that backport to older iOS | **0** | **NOT** (delegate only) |

---

## §1 Universal (already spans all generations)

These do not require a “backport” — they are era-agnostic today.

| Capability | Classes / entry | Modes |
|------------|-----------------|-------|
| DFU detect / memory R/W | `DeviceManager`, `DFUDevice` | DFU |
| Recovery detect / iBoot API | `RecoveryDevice` | Recovery |
| Normal lockdown / apps | `MobileDevice` | Normal |
| AFC upload / download | `AFCService` | Normal (post-trust) |
| Offline backup parse (v1/v2) | `MobileBackup`, `MbdbParser`, `ManifestDbParser` | Host |
| Offline Mach-O / dyld | `MachOBinary`, `DyldSharedCache`, ipswd/ipsw | Host |
| ChainRunner + JSON report | `ChainRunner`, `--report` | All (Gen6 chain on Normal only) |
| Cross-gen probes (6) | `PrimitiveRegistry` | All / mode-filtered |
| checkm8 external delegate | `Checkm8`, `Checkm8BootromPrimitive` | DFU |
| Gen 6 exploit delegate | `ExploitDelegate`, `PURPLEPOIS0N_DOPAMINE_*` | Normal (arch must match dylib) |

**Probe stage (all modes):** `OfflinePatchPrimitive`, `IpswdHostProbePrimitive`, `SandboxCapabilityProbePrimitive`, plus mode-specific Bootrom / Normal / AFC probes.

---

## §2 Framework backports (Gen 6 architecture → older eras)

Modern work mostly advances **orchestration**. These patterns backport; PUAF/kfd/dmaFail binaries do not.

| Pattern | Source (today) | Backport to | Status | Phase |
|---------|----------------|-------------|--------|-------|
| `ExploitDelegate` (`dlopen`, `exploit_init` / `exploit_deinit`) | Gen 6 / Dopamine | Gen 0 limera1n, Gen 1–4 userland JBs, Gen 5 checkra1n | **Done** (env paths differ per tool) | 6.7 |
| `ExploitModulePrimitive` + priority picker | Gen 6 registry | One probe module per historical exploit family | **Done** (Gen 6 + limera1n/24kpwn/evasi0n/checkra1n) | **7.1** |
| Era-trimmed `ChainStage` lists | `runEraChain()` | Gen 0–4: drop PAC/PPL/TrustCache; Gen 5: Bootrom-first | **Done** | **7.2** |
| `PostExploitPrimitive` stages | Gen 6 post modules | Classic: patchfind → privilege → bootstrap (no `/var/jb`) | **Partial** (era probe messages) | **7.3** |
| `ExecutionContext` bands | `iosVersionInRange()` | Per-era module tables (evasi0n, Pangu, etc.) | **Partial** | **7.1** |
| XPF / IPSW kernelcache **host** study | `legacy/modern-era/XPF`, ipswd | Any IPSW (iOS 4–18) offline | **Done** (offline) | — |
| `PURPLEPOIS0N_JB_HELPER` installer spawn | Phase 6.7 | TrollStore / Dopamine / historical GUI CLIs | **Done** | 6.7 / **7.4** |

### Era-specific chain shapes (target)

| Generation | iOS (representative) | Chain stages to run on Normal / DFU |
|------------|----------------------|-------------------------------------|
| **0** | 4.x – 5.1.1 | DFU: Bootrom → Recovery payload; Normal: Kernel → Codesign → Bootstrap |
| **1–4** | 6 – 14 | Normal: Kernel → Patchfind → Privilege → Bootstrap |
| **5** | 12 – 17 (A5–A11) | DFU: Bootrom (checkm8) → Recovery ramdisk → Bootstrap |
| **6** | 15 – 18+ | Normal: full Dopamine-shaped chain (current `kGen6Stages`) |

Implementation note: multiple chains can coexist in `ChainRunner` gated by `ExecutionContext` flags (e.g. `detectedGeneration` or iOS version ranges) — see Phase **7.2**.

---

## §3 Legacy mirror backports (historical tools → purplepois0n)

Features present in `legacy/Chronic-Dev/`, `legacy/OpenJailbreak/`, or `legacy/posixninja/` but not yet in `src/`. Study paths in [LEARNINGS.md](legacy/LEARNINGS.md).

| Feature | Era | Legacy anchor | purplepois0n | Backport | Phase |
|---------|-----|---------------|--------------|----------|-------|
| IMG3 / iBSS / iBEC upload | Gen 0–2 | `syringe/utilities/`, `gp2` payloads | `RecoveryDevice::sendFile` + upload primitive | **Partial** | **7.5** |
| `irecv_send_file` / reset helpers | Gen 0–5 | `syringe/syringe/libirecovery.c` | `sendFile`, `reset`, `reboot` | **Partial** | **7.5** |
| Crash log → userspace slide | Gen 0 (absinthe) | `absinthe-2.0` Mach-O helpers | `CrashSlideHelper`, `--analyze-crash` | **Done** (offline only) | **7.6** |
| `mobilebackup2` live client | Gen 0–1 | absinthe GUI/CLI | `MobileBackup2ProbePrimitive` | **Partial** (probe only) | **7.7** |
| AFC directory listing CLI | Gen 0–6 | absinthe staging | `AFCService::listDirectory` + CLI | **Done** | 6.8 |
| Backup summary in probe chain | Gen 0–6 | absinthe workflow | `BackupProbePrimitive` in ChainRunner | **Done** | 6.9 |
| Encrypted backup decrypt | Gen 0–6 | absinthe keybag | `isEncrypted` only | **NOT** (deferred) | 6.10 |
| iTunes / Finder pairing interference | Gen 0–1 | absinthe `iTunesKiller` | — | **Study** (doc + optional helper) | **7.8** |
| TSS / SHSH / signed restore hooks | Gen 0–4 | redsn0n ecosystem | `TssClient` + libtatsu SEP/BB, `--sep-ipsw`/`--bb-ipsw` | **Partial** | **7.10** |
| Host codesign + IPA sideload | Gen 0 / Fugu15 / yalu | absinthe IPA staging | `CodesignDelegate`, `SideloadPrimitive`, `--sign-ipa` / `--install-ipa` | **Partial** | **7.11** |
| Trust cache / pseudo-sign on device | Gen 6 | Dopamine `jbctl` | `TrustCacheDelegate`, `gen6-trustcache` | **Partial** | **7.11** |
| CPID / board-id tables in logs | Gen 0–5 | `syringe/include/libirecovery.h` | Partial (`-l`) | **Planned** | **7.9** |
| 32-bit Mach-O branch in probes | Appendix A | Phoenix / doubleH3lix | `--arch arm32` | **Done** | — |

---

## §4 No backport (exploit / mitigation layer)

Register these in the primitive taxonomy for **honest probes**; do not expect them to run on older iOS.

| Gen 6 module / concept | Approx. requirement | Backport to Gen 0–4? |
|------------------------|---------------------|----------------------|
| kfd / PUAF | iOS 15+, dangling PTE class | **NOT** |
| weightBufs / multicast_bytecopy | Fixed CVE windows ~15–16.x | **NOT** (except same builds) |
| DarkSword | Modern kernel port + ITW chain | **NOT** |
| dmaFail (PPL bypass) | PPL / AGX MMIO era | **N/A** pre-PPL |
| badRecovery (PAC bypass) | arm64e | **N/A** pre-A12 |
| PhysRw / libjailbreak map | Post-PUAF kernel R/W API | **NOT** |
| TrustCache / BaseBin | Modern codesign + rootless | **NOT** |
| Rootless bootstrap `/var/jb` | SSV / iOS 15+ | **NOT** (rootful era used `/` patches) |
| libkfd in-tree | — | **NOT** (policy) |

**Delegate exception:** a **built** historical exploit dylib (limera1n, evasi0n, checkra1n) can be loaded the same way as Dopamine if the user supplies the path and CPU architecture matches.

---

## §5 Per-generation matrix

Rows: capability. Columns: can purplepois0n use or backport it for that generation?

| Capability | Gen 0 | Gen 1–4 | Gen 5 | Gen 6 |
|------------|-------|---------|-------|-------|
| DFU / Recovery I/O | **Done** | Partial | **Done** | N/A primary |
| Normal lockdown / AFC | **Done** | **Done** | **Done** | **Done** |
| Offline backup parse | **Done** | **Done** | **Done** | **Done** |
| Offline Mach-O / dyld / IPSW | **Done** | **Done** | **Done** | **Done** |
| ChainRunner probes | **Done** | **Done** | **Done** | **Done** |
| Era-shaped execute chain | **Partial** | **Partial** | **Partial** (checkm8) | **Done** |
| Exploit delegate | **Partial** | **Partial** | **Partial** | **Partial** |
| In-tree kernel exploit | **NOT** | **NOT** | **NOT** | **NOT** |
| Untether / persistence install | **NOT** | **NOT** | **NOT** | **NOT** |
| IMG3 / ramdisk staging | **Partial** | **Partial** | **Partial** | N/A |
| Rootless bootstrap | **N/A** | **N/A** | Partial (palera1n) | Delegate |

---

## §6 Implementation phases (Phase 7)

Tracked in [INTEGRATION_PLAN.md](legacy/INTEGRATION_PLAN.md#phase-7-backport--multi-generation-chains). Suggested order:

1. **7.1** — Historical `ExploitModulePrimitive` stubs — **Done** (limera1n, 24kpwn, evasi0n, checkra1n).
2. **7.2** — `ChainRunner::runEraChain()` + `detectJailbreakGeneration()` — **Done**.
3. **7.3** — Post-exploit stage adapters — **Partial** (era probe messages).
4. **7.4** — `PURPLEPOIS0N_JB_HELPER` — **Done**.
5. **7.5** — Recovery IMG3 / signed blob upload API (study `syringe`, no blob vendoring).
6. **7.12** — In-memory HFS+ ramdisk builder + Recovery multi-stage chain — **Done** ([book/deep/recovery-ramdisk.md](book/deep/recovery-ramdisk.md), [validation/ramdisk-recovery-smoke.md](validation/ramdisk-recovery-smoke.md)).
7. **7.6** — Offline crash-log slide helper — **Done** (`--analyze-crash`).
8. **7.10** — TSS probe + libtatsu SEP/BB — **Partial** (host `make smoke-tss`; hardware validation pending).
9. **7.11** — Host codesign + sideload + trustcache — **Partial** (`--post-jb-pipeline`, ramdisk SSH trustcache; [validation/sideload-trustcache-smoke.md](validation/sideload-trustcache-smoke.md)).
10. **7.7** — mobilebackup2 probe — **Partial** (connect + version only).
11. **7.8–7.9** — pairing notes, extended CPID logging.

**Acceptance:** `--gen0` on DFU runs Gen 5-shaped chain; on Normal with iOS &lt; 15 runs trimmed chain without PAC/PPL stages; [SUPPORT.md](SUPPORT.md) and [COMPARISON_MATRIX.md](legacy/COMPARISON_MATRIX.md) updated.

---

## Maintenance

Update this matrix when:

- A Phase 7 task lands in `src/`
- A new primitive is registered in `PrimitiveRegistry.cpp`
- [SUPPORT.md](SUPPORT.md) capability rows change
- Gen 6 delegate env vars expand

---

## Related documents

| Doc | Role |
|-----|------|
| [GENERATIONS.md](GENERATIONS.md) | Era timelines, mitigations, component mapping |
| [SUPPORT.md](SUPPORT.md) | Gen 0 honest capability matrix |
| [legacy/COMPARISON_MATRIX.md](legacy/COMPARISON_MATRIX.md) | greenpois0n vs absinthe vs today |
| [legacy/LEARNINGS.md](legacy/LEARNINGS.md) | Safe-to-port vs do-not-port from mirrors |
| [legacy/MODERN_ERA_LEARNINGS.md](legacy/MODERN_ERA_LEARNINGS.md) | Gen 6 mirror synthesis |
| [book/deep/primitives-gen0.md](book/deep/primitives-gen0.md) | ChainRunner / Gen6 implementation tour |
| [legacy/INTEGRATION_PLAN.md](legacy/INTEGRATION_PLAN.md) | Phased roadmap including Phase 7 |
| [legacy/PHASE_STATUS.md](legacy/PHASE_STATUS.md) | Living phase rollup |
