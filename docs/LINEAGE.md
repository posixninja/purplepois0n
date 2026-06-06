# Lineage: greenpois0n, absinthe, and purplepois0n

purplepois0n is the spiritual successor to **greenpois0n** and **absinthe** from the Chronic Dev Team era. Those tools shipped complete jailbreaks for specific iOS builds. This repository ships a **host-side framework**—device I/O, parsers, and exploit hooks—without bundled exploits.

## Predecessors

### greenpois0n (~2010–2011)

- **Team:** Chronic Dev Team (with contributions from the broader community, including bootrom work bundled with tools like limera1n).
- **iOS range:** Primarily **4.x** (representative builds include 4.1, 4.2.1).
- **Characteristics:**
  - Combined **DFU / bootrom** exploitation paths with userland components.
  - Often **tethered** or **semi-tethered** depending on device and build.
  - Established the pattern of a desktop tool talking USB to a device in Recovery or DFU, then finishing in normal iOS.

### absinthe (January 2012)

- **Team:** Chronic Dev Team and pod2g.
- **iOS range:** **5.0.1** untethered (initial release); **Absinthe 2.0** (May 2012) extended coverage to **5.1.1**.
- **Characteristics:**
  - One of the last high-profile **single-release untethers** for a whole minor iOS generation.
  - Used a **backup-domain** foothold (mobile backup / sync path) rather than only browser or PDF vectors—without reproducing payload details here.
  - Normal-mode staging and persistence after kernel compromise.

### Related tools in the same generation

| Tool | Notes |
|------|--------|
| **redsn0w** | Long-lived utility; tethered/semi paths across many builds |
| **Corona** | Untether complement for **5.0.1** (pod2g) |
| **limera1n** | Bootrom-class exploit used alongside DFU workflows of the era |

## What changed after absinthe

The jailbreak scene did not “end”—it **specialized**:

- **Version lock-in:** Public releases targeted narrow iOS build ranges instead of entire major versions.
- **64-bit (A7, iOS 7):** Userspace and kernel exploit techniques had to account for new ABI and later arm64e.
- **Semi-untethered norm (iOS 9.3.3 onward for many tools):** Re-run an app after reboot to re-apply kernel patches.
- **Mitigation stack:** KTRR, PAC, PPL/SPTM, sealed system volume, and hardened heaps made multi-bug chains standard.
- **Hardware assist (checkm8, 2019):** Unpatchable bootrom bug on A5–A11 enables tethered bootstrap on many iOS versions—but not a full untether by itself.
- **Rootless bootstraps (Dopamine era):** System partition is no longer the install target; package managers and tweak injection moved to `/var` and bind mounts (Procursus, ElleKit, roothide). **Dopamine 2.x** relies on public **PUAF/kfd** kernel primitives—see [book/deep/puaf-kfd-era.md](book/deep/puaf-kfd-era.md).

Tool names evolved through **evad3rs**, **Pangu**, **TaiG**, **Luca Todesco (yalu)**, **Coolstar (Electra/Chimera/Odyssey/Taurine)**, **Pwn20wnd (unc0ver)**, **checkra1n / palera1n**, and **opa334 (Dopamine)**—cataloged in [GENERATIONS.md](GENERATIONS.md).

## purplepois0n’s role today

purplepois0n mirrors the **shape** of classic jailbreak utilities:

```
Host tool (purplepois0n)
    → DeviceManager detects Normal | Recovery | DFU
    → Mode-specific backend (MobileDevice | RecoveryDevice | DFUDevice)
    → Optional AFC, backup parsing, Mach-O / dyld analysis
    → performJailbreak() dispatches exploit logic (placeholder today)
```

| Aspect | Status in this repo |
|--------|---------------------|
| USB device detection & enumeration | Implemented (`DeviceManager`) |
| Normal / Recovery / DFU I/O | Implemented (mode classes) |
| AFC file operations | Implemented (`AFCService`) |
| Backup manifest parsing | Implemented (`MobileBackup`) |
| dyld shared cache / Mach-O parsing | Implemented (`DyldSharedCache`, `MachOBinary`; ipswd / ipsw) |
| Exploit chains & bootstraps | **Not implemented** (see [SUPPORT.md](SUPPORT.md), `Gen0Workflow`) |

purplepois0n is a **framework for research and extension**, not a released jailbreak for a specific iOS version. Contributors plug exploits into [`src/purplepois0n.cpp`](../src/purplepois0n.cpp); see [GENERATIONS.md](GENERATIONS.md) for which components align with each historical era.

## Lineage diagram

High-level flow of public jailbreak *eras* (tool names only—not exploit steps):

```mermaid
flowchart LR
  greenpois0n --> absinthe
  absinthe --> evasi0n
  evasi0n --> PanguTaiG
  PanguTaiG --> yaluEra
  yaluEra --> unc0verElectra
  unc0verElectra --> checkra1n
  checkra1n --> DopamineRootless
  DopamineRootless --> purplepois0n
```

- **PanguTaiG:** Pangu, TaiG, iOS 7–9 era  
- **yaluEra:** yalu, h3lix, Meridian, iOS 10  
- **unc0verElectra:** Electra, unc0ver, Chimera, Odyssey, Taurine, iOS 11–14  
- **DopamineRootless:** Dopamine, **PUAF/kfd**, rootless bootstraps, iOS 15+  
- **purplepois0n:** This repository (framework, not a shipping jailbreak)

## Next steps

- Full generation tables, mitigations, and per-era framework mapping: [GENERATIONS.md](GENERATIONS.md)  
- Local GitHub org mirrors (Chronic-Dev, OpenJailbreak, posixninja): [ARCHIVES.md](ARCHIVES.md)  
- Build and CLI usage: [../README.md](../README.md)
