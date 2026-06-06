# Jailbreak generations reference

This document catalogs **representative** public jailbreak eras from the Chronic Dev Team generation through modern rootless tooling. Lists are not exhaustive; they highlight tools and trends that shaped each period.

For purplepois0n’s origin story, see [LINEAGE.md](LINEAGE.md).

---

## Generation 0: Chronic Dev era (predecessors)

**Years:** ~2010–2012  
**iOS range:** **4.x – 5.1.1**

### Representative tools

| Tool | Team | iOS versions | Jailbreak type |
|------|------|--------------|----------------|
| greenpois0n | Chronic Dev Team | 4.1, 4.2.1 (representative) | Tethered / semi / bundled DFU |
| absinthe | Chronic Dev Team, pod2g | 5.0.1 | Untethered |
| Absinthe 2.0 | Chronic Dev Team, pod2g | 5.1.1 | Untethered |
| redsn0w | iPhone Dev Team / community | 4.x–6.x (varies) | Tethered / semi |
| Corona | pod2g | 5.0.1 | Untethered (companion) |
| limera1n | geohot | 4.x era devices (A4-class) | Bootrom / DFU |
| 24kpwn (0x24000) | planetbeing / Chronic Dev | 3.x–4.x old-BR 3GS, iPod 2G | Bootrom untether (DFU IMG3) |

### Jailbreak type

Primarily **untethered** (absinthe) or **tethered/semi-tethered** (greenpois0n, redsn0w) depending on device and build.

### Security landscape

- **ASLR** in userspace; kernel slide still limited compared to later eras.
- **DFU bootrom** issues on A4-class hardware enabled low-level entry for some devices.
- Backup and sync trust boundaries were less hardened—relevant to absinthe-era **backup-domain** research (conceptual only here).
- No PAC, no KTRR, no rootless system layout.

### Typical chain shape (conceptual)

1. Initial foothold: DFU bootrom path **or** userland / backup-domain trigger in normal iOS.  
2. Kernel memory corruption or patch to gain code execution in kernel.  
3. Patch `amfid`, mount roots, install bootstrap (Cydia, substrate).  
4. **Untether:** persist kernel patches across reboot (absinthe); **tethered:** repeat host tool on boot (greenpois0n patterns).

### purplepois0n mapping

| Component | Role | Status |
|-----------|------|--------|
| [`DFUDevice`](../src/DFUDevice.cpp) | Host-side DFU USB I/O for bootrom-era workflows | Implemented |
| [`RecoveryDevice`](../src/RecoveryDevice.cpp) | Recovery / iBoot-stage communication | Implemented |
| [`MobileDevice`](../src/MobileDevice.cpp) | Normal-mode USB via lockdown | Implemented |
| [`MobileBackup`](../src/MobileBackup.cpp) | Parse backup manifests and domains for research | Implemented |
| [`AFCService`](../src/AFCService.cpp) | Post-exploit file staging in normal mode | Implemented |
| `Gen0Workflow` / `performJailbreak()` | Gen 0 scaffold + gap logging | **Scaffold** (no exploits) |
| Exploit dispatch (limera1n, absinthe, untether) | — | **NOT** — see [SUPPORT.md](SUPPORT.md) |

---

## Generation 1: evad3rs era

**Years:** 2013–2014  
**iOS range:** **6.0 – 7.0.6**

### Representative tools

| Tool | Team | iOS versions | Jailbreak type |
|------|------|--------------|----------------|
| evasi0n | evad3rs | 6.0 – 6.1.2 | Untethered |
| p0sixspwn | winocm, iH8sn0w, et al. | 6.1.3 – 6.1.6 | Untethered (older devices) |
| evasi0n7 | evad3rs | 7.0 – 7.0.6 | Untethered |

### Jailbreak type

**Untethered** mainstream releases; among the last era where a single team could ship a broad untether for a major iOS release.

### Security landscape

- Stronger **ASLR** and sandboxing in userspace.
- **A7 / arm64** arrives with iPhone 5s (iOS 7)—64-bit userspace and kernel paths diverge from 32-bit.
- Apple begins tightening **code signing** enforcement (AMFI) and kernel patch protection precursors.
- Exploit chains still often **2–3 stages** (userland → kernel → persistence) without PAC/PPL.

### Typical chain shape (conceptual)

1. Userland vulnerability (app, service, or race) for initial code execution.  
2. Kernel exploit or info leak + kernel bug for ring-0.  
3. Patch kernel, install untether payload and package manager bootstrap.

### purplepois0n mapping

| Component | Role | Status |
|-----------|------|--------|
| [`MobileDevice`](../src/MobileDevice.cpp) | Normal-mode staging | Implemented |
| [`AFCService`](../src/AFCService.cpp) | Push payloads / collect artifacts | Implemented |
| [`MachOParser`](../src/MachOParser.cpp) | Inspect binaries for symbols/segments | Implemented |
| `performJailbreak()` Normal | Userland-first chains | **TODO** |

---

## Generation 2: Pangu / TaiG era

**Years:** 2014–2016  
**iOS range:** **7.1 – 9.3.3** (representative)

### Representative tools

| Tool | Team | iOS versions | Jailbreak type |
|------|------|--------------|----------------|
| Pangu | Pangu Team | 7.1 – 7.1.2 | Untethered |
| Pangu8 | Pangu Team | 8.0 – 8.1 | Untethered |
| TaiG | TaiG | 8.0 – 8.4 | Untethered |
| Pangu9 | Pangu Team | 9.0 – 9.1 | Untethered |
| Pangu9 (9.2–9.3.3) | Pangu Team | 9.2 – 9.3.3 | Semi-untethered |
| PP Jailbreak | Various | 8.x (regional) | Semi / untether (varies) |

### Jailbreak type

Early builds **untethered**; **semi-untethered** becomes common (re-run app after reboot) especially on **9.3.3**.

### Security landscape

- **dyld shared cache** is central to userspace layout—attacks and research target cache-aware primitives.
- **KASLR** and tighter sandbox profiles increase need for info leaks.
- **Kext patching** and substrate/substitute injection drive “patchfinder” tooling.
- 32-bit vs 64-bit split: many tools target one architecture per release.

### Typical chain shape (conceptual)

1. Userland exploit (often app or system service).  
2. Kernel exploit with slide defeat.  
3. Install **semi-untether** app or untether payload; bootstrap tweaks via Cydia/Substrate ecosystem.

### purplepois0n mapping

| Component | Role | Status |
|-----------|------|--------|
| [`DyldCacheParser`](../src/DyldCacheParser.cpp) | Parse shared cache for research (iOS 8+ layout) | Implemented |
| [`MachOParser`](../src/MachOParser.cpp) | Mach-O segments, load commands | Implemented |
| [`MobileDevice`](../src/MobileDevice.cpp) + [`AFCService`](../src/AFCService.cpp) | Semi-untether payload delivery | Implemented (I/O only) |
| `performJailbreak()` Normal | Semi-untether re-trigger hook point | **TODO** |

---

## Generation 3: iOS 10 semi-untether era

**Years:** 2016–2018  
**iOS range:** **10.x**

### Representative tools

| Tool | Team | iOS versions | Jailbreak type |
|------|------|--------------|----------------|
| yalu / yalu102 | Luca Todesco | 10.0 – 10.2 (varies by build) | Semi-untethered |
| extra_recipe, g0blin | Community / Luca forks | 10.x subsets | Semi-untethered |
| h3lix, doubleH3lix | Community | 10.x (32-bit) | Semi-untethered |
| Meridian | Sparkey, Ian Beer, et al. | 10.3.3 (64-bit) | Semi-untethered |
| Saïgon | Abraham Masri | 10.2.1 (subset) | Semi-untethered |

### Jailbreak type

Almost exclusively **semi-untethered** on supported builds.

### Security landscape

- **KTRR** (Kernel Text Readonly Region) on arm64 devices—kernel code pages harder to patch persistently.
- Stricter **AMFI** and code signing; more **version-locked** exploits.
- Apple deprecates many classic injection surfaces; patchfinders and vnode-based paths proliferate in the scene.

### Typical chain shape (conceptual)

1. Userland exploit (often app-based installer).  
2. Kernel exploit; patches may live in **userland daemon** re-applied on each boot.  
3. User taps jailbreak app after reboot (**semi-untether**).

### purplepois0n mapping

| Component | Role | Status |
|-----------|------|--------|
| `performJailbreak()` Normal | Semi-untether “run after reboot” integration point | **TODO** |
| [`DyldCacheParser`](../src/DyldCacheParser.cpp) / [`MachOParser`](../src/MachOParser.cpp) | Binary/cache analysis for patchfinding research | Implemented |
| [`AFCService`](../src/AFCService.cpp) | Artifact transfer | Implemented |

---

## Generation 4: unc0ver / Coolstar era

**Years:** 2018–2021  
**iOS range:** **11 – 14.x**

### Representative tools

| Tool | Team | iOS versions | Jailbreak type |
|------|------|--------------|----------------|
| LiberiOS | Community | 11.x (early) | Semi / dev-focused |
| Electra | Coolstar | 11.0 – 11.1.2 | Semi-untethered |
| unc0ver | Pwn20wnd | 11 – 14.8 (build-dependent) | Semi-untethered |
| Chimera | Coolstar | 12.0 – 12.5.7 | Semi-untethered |
| Odyssey | Coolstar | 13.0 – 13.7 | Semi-untethered |
| Taurine | Coolstar | 14.0 – 14.3 | Semi-untethered |
| unc0ver + Fugu14 | Pwn20wnd + community | 14.3 – 14.8 (subset) | Semi-untethered |

### Jailbreak type

**Semi-untethered** dominant; debate over **rootless** vs traditional rootful installs begins late in this era.

### Security landscape

- **PAC** (Pointer Authentication Codes) on **arm64e** (A12+)—corruption primitives need signing gadgets or bypasses.
- **AMFI**, **codesign** hardening, and **SEP** boundaries tighten persistence options.
- **Substitute** vs **Substrate**, **Sileo** vs **Cydia**—bootstrap fragmentation.
- **CVE-driven** drops increasingly version-specific.

### Typical chain shape (conceptual)

1. Userland exploit or multi-bug chain (sometimes app installer).  
2. Kernel exploit with PAC-aware primitives on arm64e.  
3. Bootstrap package manager and tweak loader; semi-untether app re-applies patches.

### purplepois0n mapping

| Component | Role | Status |
|-----------|------|--------|
| `performJailbreak()` Normal | Primary hook for semi-untether style flows | **TODO** |
| [`MachOParser`](../src/MachOParser.cpp) | arm64e binary inspection (PAC-aware research) | Implemented |
| [`DyldCacheParser`](../src/DyldCacheParser.cpp) | Shared cache analysis | Implemented |
| [`AFCService`](../src/AFCService.cpp) | Payload and log transfer | Implemented |
| Package managers (Cydia/Sileo) | Out of repo scope | Not included |

---

## Generation 5: checkm8 hardware era

**Years:** 2019–present  
**iOS range:** **12 – 17+** (device-dependent, **A5–A11** for checkm8-class hardware)  
**Devices:** iPhone 4s through iPhone X (representative); not available on A12+ for bootrom bug.

### Representative tools

| Tool | Team | iOS versions | Jailbreak type |
|------|------|--------------|----------------|
| checkra1n | checkra1n team | 12 – 14+ (A5–A11) | Hardware-assisted, tethered bootstrap |
| palera1n | palera1n team | 15 – 17+ (A8–A11) | Hardware-assisted, tethered / semi-tethered workflows |

### Jailbreak type

**Hardware-assisted (checkm8)** + **tethered** bootstrap each boot (or semi-tethered helper on some palera1n workflows). Not a traditional untether on modern iOS.

### Security landscape

- **Unpatchable bootrom** bug gives DFU-stage code execution on affected SoCs—Apple cannot fix in software for that hardware generation.
- **SEP**, **APFS**, and **passcode state** still limit what each boot achieves.
- Later iOS versions add **PAC**, **PPL**, **SPTM**—chain complexity moves to after bootrom.
- **A12+** devices are out of scope for checkm8; different tools (see Generation 6) apply.

### Typical chain shape (conceptual)

1. Device enters **DFU**; host runs checkm8 to execute bootrom payload.  
2. Load patched iBoot / ramdisk / boot chain (tool-specific).  
3. Boot into iOS with bootstrap installed; user may need to re-run on each power cycle.

### purplepois0n mapping

| Component | Role | Status |
|-----------|------|--------|
| [`DFUDevice`](../src/DFUDevice.cpp) | USB DFU communication surface | Implemented |
| [`RecoveryDevice`](../src/RecoveryDevice.cpp) | Recovery-mode operations | Implemented |
| checkm8 exploit itself | External to this repo | **Not implemented** |
| `performJailbreak()` DFU / Recovery | Intended integration point for bootrom-stage helpers | **TODO** |

purplepois0n provides **host I/O** only; integrating checkm8 or palera1n would require separate exploit modules and ramdisk assets.

---

## Generation 6: Rootless modern era

**Years:** 2021–present  
**iOS range:** **15 – 18+** (build- and device-specific)

### Representative tools

| Tool | Team | iOS versions | Jailbreak type |
|------|------|--------------|----------------|
| Fugu15 / Fugu15 Max | Linus Henze | 15.x research chains | Research / semi paths |
| Dopamine | opa334 | 15 – 16.x stable; **2.5b3** arm64 17–18.7.x (beta) | Rootless semi-untethered |
| Dopamine 2 | opa334 | Same — use [releases](https://github.com/opa334/Dopamine/releases) + [§3.1 matrix](book/deep/modern-era-web-sources.md#31-dopamine-support-matrix-release-sourced) | Rootless semi-untethered |
| XinaA15 | Community | 15 (A12+, brief) | Rootless (abandoned) |
| palera1n | palera1n team | 15+ on A8–A11 | Hardware + rootless/hybrid workflows |

### Jailbreak type

**Rootless** semi-untethered is the norm on many recent releases—modifications under `/var` with bind mounts instead of writable system partition.

### Security landscape

- **Sealed System Volume (SSV)** and read-only system layout—rootful installs largely obsolete.
- **PPL**, **SPTM**, **TXM** on newer devices—kernel page protections beyond classic KTRR.
- **PUAF / kfd era (2023+):** Physical use-after-free via dangling PTEs → **KRKW** (`libkfd`); Apple patched CVE-2023-23536 (PhysPuppet), CVE-2023-32434 (Smith), CVE-2023-41974 (Landa) in point releases.
- **Dopamine 2.x** adds exploit picker (kfd + **dmaFail** PPL bypass + **weightBufs**, **multicast_bytecopy**, **DarkSword**), **XPF** patchfinder, launchd hook bootstrap.
- **Procursus**, **ElleKit**, **roothide** replace classic Cydia + Substrate on many setups.
- Exploits are highly **build-specific**; public window often closes quickly.

### Typical chain shape (conceptual)

**Dopamine 2 / PUAF path (representative):**

1. PUAF primitive (dangling PTEs) via libkfd method or successor module.
2. Kernel R/W (`kread`/`kwrite`).
3. **PPL bypass** (e.g. **dmaFail**, CVE-2023-38606) where still applicable.
4. XPF / tool-specific patchfinding; PAC helpers on arm64e.
5. Rootless bootstrap under `/var/jb`; semi-untether re-run where applicable.

**Dopamine 1 / arm64e path (representative):** oobPCI → PAC/PPL helpers → bootstrap (see [book/deep/puaf-kfd-era.md](book/deep/puaf-kfd-era.md)).

### purplepois0n mapping

| Component | Role | Status |
|-----------|------|--------|
| [`AFCService`](../src/AFCService.cpp) | Transfer rootless payloads / logs from host | Implemented |
| [`MachOBinary`](../src/MachOBinary.cpp) / [`DyldSharedCache`](../src/DyldSharedCache.cpp) | IPSW / kernelcache research (ipswd / ipsw) | Implemented |
| PUAF / kfd / Dopamine exploit modules | On-device kernel entry | **Out of repo** |
| `performJailbreak()` Normal | Semi-untether / installer hook | **TODO** |
| Procursus / Sileo / ElleKit install | Bootstrap packaging | **Out of repo scope** |

**Book:** [book/deep/puaf-kfd-era.md](book/deep/puaf-kfd-era.md) — PUAF, libkfd, Dopamine 2 architecture (conceptual). **Web sources:** [book/deep/modern-era-web-sources.md](book/deep/modern-era-web-sources.md).

---

## Appendix A: 32-bit and legacy devices

**Years:** 2016–2018 (overlaps Generation 2–3)  
**iOS range:** **9.3.5**, **10.3.3** on older 32-bit hardware

### Representative tools

| Tool | Team | iOS versions | Jailbreak type |
|------|------|--------------|----------------|
| Phoenix | Siguza, tihmstar | 9.3.5 (32-bit) | Semi-untethered |
| Home Depot | Community | 9.x (32-bit subset) | Semi-untethered |
| doubleH3lix | Community | 10.x (32-bit) | Semi-untethered |

### Security landscape

32-bit devices lack arm64e PAC but also lack ongoing Apple support—tools are **frozen** to last compatible iOS.

### purplepois0n mapping

Same as Generation 2–3: [`MachOParser`](../src/MachOParser.cpp) for **32-bit Mach-O** research, Normal-mode `performJailbreak()` **TODO**. No separate 32-bit code path in the framework today—contributors would branch on architecture in exploit modules.

---

## Jailbreak types explained

| Type | Behavior after reboot |
|------|------------------------|
| **Untethered** | Device boots already jailbroken; no host PC required. |
| **Semi-untethered** | Device boots stock iOS; user re-runs a jailbreak app (or host tool) to re-apply patches. |
| **Tethered** | Device cannot boot into usable jailbroken state without a host PC each boot. |
| **Hardware-assisted** | Bootrom bug (e.g. checkm8) enables low-level entry; often paired with tethered or semi-tethered bootstrap. |
| **Rootless** | Jailbreak does not require writable system partition; files live under `/var` with bind mounts. |

---

## Mitigation timeline

| Mitigation | Approx. introduction | Effect on chains |
|------------|----------------------|------------------|
| ASLR | iOS 4+ | Randomized userspace load addresses |
| KASLR | iOS 6+ | Kernel slide; requires leaks |
| AMFI / codesign | Strengthened over time | Blocks unsigned code execution |
| KTRR | iOS 10+ (arm64) | Kernel text not trivially patchable |
| PAC (arm64e) | A12+ | Signed pointers; corrupt targets need gadgets |
| PPL / SPTM / TXM | iOS 14–17+ | Page table and kernel protection layers |
| Sealed System Volume | iOS 15+ | System partition integrity; drives rootless |
| Hardened heap (kalloc_type, etc.) | iOS 15+ | Fewer classic kernel overflows; VM/PTE bugs (PUAF class) gain prominence |
| PUAF CVE patches (PhysPuppet/Smith/Landa) | iOS 16.4–17.0 point releases | Closes specific dangling-PTE primitives used by kfd |

---

## What purplepois0n does not cover

- **SEP** (Secure Enclave) compromise or payment/ biometrics bypass research tooling  
- **Baseband** unlocks or cellular stack exploitation  
- **iCloud Activation Lock** or FMIP bypass  
- **App Store** policy, enterprise cert abuse, or sideloading marketplaces  
- Shipping **Cydia**, **Sileo**, or full **bootstrap** installers (use external projects)  
- **checkm8** implementation, ramdisks, or boot chain patches (integration left to contributors)  
- Step-by-step instructions to build or deploy weaponized exploits  

---

## Quick reference: framework vs era

| Era | Primary `DeviceState` | Key classes |
|-----|----------------------|-------------|
| Gen 0 (greenpois0n / absinthe) | DFU, Recovery, Normal | `DFUDevice`, `RecoveryDevice`, `MobileBackup`, `AFCService` |
| Gen 1–4 (evasi0n → Taurine) | Normal | `MobileDevice`, `AFCService`, `MachOParser`, `DyldCacheParser` |
| Gen 5 (checkra1n / palera1n) | DFU, Recovery | `DFUDevice`, `RecoveryDevice` |
| Gen 6 (Dopamine / PUAF) | Normal | `AFCService`, `MachOBinary`, `DyldSharedCache`; kfd/Dopamine external |

All eras: exploit entry [`performJailbreak()`](../src/purplepois0n.cpp) — **placeholder**.

---

See also: [LINEAGE.md](LINEAGE.md) · [BACKPORT_MATRIX.md](BACKPORT_MATRIX.md) · [docs index](README.md) · [Project README](../README.md)
