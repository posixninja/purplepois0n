# Chapter 2: evad3rs — evasi0n & evasi0n7

**Depth TOC:** [L0](#l0--summary) · [L1](#l1--history) · [L2](#l2--ecosystem) · [L3](#l3--security-engineering) · [L4](#l4--host-tooling-architecture) · [L5](#l5--purplepois0n-this-era) · [L6](#l6--sources--further-reading)

## L0 — Summary

evasi0n (2013) and evasi0n7 (2014) restored the **one-click desktop jailbreak** model for iOS 6–7 after the backup-heavy Absinthe era—**untethered**, userland-led, with no DFU requirement for mainstream paths and major community friction around evasi0n7’s TaiG bundling controversy.

## L1 — History

| Field | Detail |
|-------|--------|
| **Years** | 2013–2014 |
| **iOS** | evasi0n: 6.0–6.1.2; evasi0n7: 7.0–7.0.6 |
| **Team** | **evad3rs** — pod2g, MuscleNerd, planetbeing, pimskeks |
| **Impact** | >7M downloads in four days (Wikipedia) |

| Tool | Jailbreak type | Notes |
|------|----------------|-------|
| evasi0n | **Untethered** | Cydia package could untether existing tethered installs |
| evasi0n7 | **Untethered** | A7/64-bit; TaiG bundling on Chinese-locale systems (later removed) |
| p0sixspwn | **Untethered** | Community fork 6.1.3–6.1.6 (see GENERATIONS.md) |

Apple patched evasi0n bugs in **iOS 6.1.3**; evasi0n7 window closed by **iOS 7.1**.

## L2 — Ecosystem

| Aspect | evasi0n era | vs Absinthe |
|--------|-------------|-------------|
| **Host UX** | Single executable, minimal user steps | Backup restore + web clip |
| **Store** | Cydia primary; Substrate stable on 6.x | Same Cydia, different delivery |
| **7.x friction** | Cydia/Substrate lag at evasi0n7 launch; public letters (saurik vs evad3rs) | Mature 5.x Cydia |
| **Forks** | p0sixspwn for stranded 6.1.3–6.1.6 devices | N/A |
| **Open source** | evasi0n6 published 2017 (OpenJailbreak) | Mostly closed during peak |

**Patchfinder culture:** Wikis tie evasi0n to later offset-hunting tooling—relevant to host-side Mach-O/dyld analysis, not on-device DFU.

## L3 — Security engineering

**Mitigations**

- Mature **ASLR** and tighter sandboxing.
- **A7 / arm64** (iPhone 5s) splits research from 32-bit.
- Build-specific patch windows (6.1.3, 7.1).

**Chain shape (conceptual)**

1. **Userland** vulnerability in reachable service/app context.
2. **Kernel** bug or info leak + ring-0.
3. Patch kernel, install **untether** and Cydia bootstrap.
4. No DFU for mainstream evasi0n paths.

## L4 — Host tooling architecture

| Mode | Typical evasi0n stack | purplepois0n analogue |
|------|----------------------|---------------------|
| Normal USB | usbmuxd → lockdown → optional AFC | `MobileDevice`, `AFCService` |
| DFU | Rare for these releases | `DFUDevice` unused in mainstream path |
| Binary analysis | Host researchers inspect `dyld`/`mach-o` from IPSW | `MachOParser`, `DyldCacheParser` |

```mermaid
flowchart LR
    Host[evasi0n binary]
    USB[usbmuxd]
    iOS[iOS userland]
    Kern[kernel patch]
    Cydia[Cydia bootstrap]
    Host --> USB --> iOS --> Kern --> Cydia
```

Desktop targets: Windows, macOS, Linux (wiki). Public site `evasi0n.com` (archive recommended).

## L5 — purplepois0n (this era)

**Branch:** `DeviceState::Normal`.

| Component | Status |
|-----------|--------|
| [`MobileDevice`](../../src/MobileDevice.h) | **Implemented** — primary staging |
| [`AFCService`](../../src/AFCService.h) | **Implemented** — artifacts/payloads |
| [`MachOParser`](../../src/MachOParser.h) | **Implemented** — binary inspection |
| [`DyldCacheParser`](../../src/DyldCacheParser.h) | Partial era relevance (cache centralizes later) |
| `performJailbreak()` Normal | **TODO** |

Contributor pattern: use `MobileDevice::getValue` for `ProductVersion` gating, AFC for logs, `MachOParser` on extracted binaries—hook exploit module from Normal branch.

**Deep dives:** [normal-mode-afc-backup.md](deep/normal-mode-afc-backup.md), [binary-parsers.md](deep/binary-parsers.md)

[GENERATIONS.md — Generation 1](../GENERATIONS.md#generation-1-evad3rs-era)

## L6 — Sources & further reading

| Type | URL |
|------|-----|
| iPhone Wiki — evasi0n | https://www.theiphonewiki.com/wiki/Evasi0n |
| evasi0n7 | https://www.theiphonewiki.com/wiki/Evasi0n7 |
| Wikipedia | https://en.wikipedia.org/wiki/Evasi0n |
| Open source | https://github.com/OpenJailbreak/evasi0n6 |
| evad3rs letter | https://ijunkie.com/evad3rs-letter-taig-chinese-app-store-saurik/ |

**Not found:** Official evad3rs whitepaper; DEF CON “evasi0n” talk; maintained first-party evasi0n7 chain repo.

**Archive.org:** https://evasi0n.com/iOS6 (wiki reference)

**Legacy integration docs (purplepois0n):** [LEARNINGS.md](../legacy/LEARNINGS.md) · [REPO_INDEX.md](../legacy/REPO_INDEX.md) · [INTEGRATION_PLAN.md](../legacy/INTEGRATION_PLAN.md) · [COMPARISON_MATRIX.md](../legacy/COMPARISON_MATRIX.md) · [PHASE_STATUS.md](../legacy/PHASE_STATUS.md)
