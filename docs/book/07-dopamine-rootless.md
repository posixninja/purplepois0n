# Chapter 7: Dopamine & rootless modern era

**Depth TOC:** [L0](#l0--summary) · [L1](#l1--history) · [L2](#l2--ecosystem) · [L3](#l3--security-engineering) · [L4](#l4--host-tooling-architecture) · [L5](#l5--purplepois0n-this-era) · [L6](#l6--sources--further-reading)

## L0 — Summary

Dopamine (2023+) typifies **rootless semi-untethered** jailbreaks on iOS 15–16 for arm64/arm64e: state under `/var/jb`, **TrollStore** install paths, Procursus/ElleKit/Sileo ecosystem—while purplepois0n offers Normal-mode USB research hooks without bundling exploits or bootstrap packages.

## L1 — History

| Field | Detail |
|-------|--------|
| **Years** | 2023–present (active on GitHub 2026) |
| **iOS** | 15.0–16.6.1 (arm64/arm64e ranges differ—see Dopamine README) |
| **Lead** | **opa334** |
| **Type** | **Rootless semi-untethered** |

Related: Fugu15 (research), XinaA15 (brief A12+), palera1n on checkm8 hardware (Ch. 6).

## L2 — Ecosystem

| Aspect | Rootless era |
|--------|----------------|
| **Prefix** | `/var/jb` (roothide may relocate) |
| **Bootstrap** | Procursus packages |
| **Tweaks** | ElleKit injection |
| **Store** | Sileo common |
| **Install** | TrollStore perma-signing frequent |
| **vs rootful** | No writable sealed system volume |
| **Host role** | Less central than checkra1n; USB for logs/sideload support |

Press cites **KFD**-class primitives and newer **PPL bypass** research at high level—details in source/Discord, not fully in public papers.

## L3 — Security engineering

**Mitigations**

- **SSV** — system partition integrity.
- **PPL / SPTM / TXM** on newer devices.
- **kalloc_type** hardened heaps (see GENERATIONS).
- Short public exploit windows.

**Chain shape (conceptual)**

1. Userland/hybrid → kernel R/W.
2. PPL bypass on arm64e where required.
3. Bootstrap under `/var/jb`; bind mounts for tweaks.
4. Semi-untether app re-applies on boot.

## L4 — Host tooling architecture

| Path | Role |
|------|------|
| TrollStore / AltStore / Xcode | Install Dopamine IPA without classic host exploit |
| usbmux + AFC | Logs, research files (`AFCService`) |
| IPSW research | Offline `MachOParser` / `DyldCacheParser` |
| DFU | checkm8 lane only (Ch. 6)—not Dopamine’s A12+ phones |

```mermaid
flowchart LR
    User[User]
    TS[TrollStore]
    App[Dopamine app]
    VarJB["/var/jb bootstrap"]
    User --> TS --> App --> VarJB
```

Guides: ios.cfw.guide Dopamine + TrollStore pages (install architecture, not exploits).

## L5 — purplepois0n (this era)

**Branch:** `DeviceState::Normal`.

| Component | Status |
|-----------|--------|
| `performJailbreak()` Normal | **TODO** — semi-untether / installer hook |
| [`AFCService`](../../src/AFCService.h) | **Implemented** — logs/payloads |
| [`MachOParser`](../../src/MachOParser.h) / [`DyldCacheParser`](../../src/DyldCacheParser.h) | **Implemented** — firmware research |
| [`MobileDevice`](../../src/MobileDevice.h) | **Implemented** — `ProductVersion` gating |
| Procursus / Sileo / ElleKit | **Out of repo** |

No arm64e-specific branch in `DeviceState`—contributors gate inside Normal exploit module using `getDeviceType()` + parser CPU type.

**Deep dives:** [normal-mode-afc-backup.md](deep/normal-mode-afc-backup.md), [binary-parsers.md](deep/binary-parsers.md)

[GENERATIONS.md — Generation 6](../GENERATIONS.md#generation-6-rootless-modern-era)

## L6 — Sources & further reading

| Type | URL |
|------|-----|
| Dopamine GitHub | https://github.com/opa334/Dopamine |
| Official site | https://ellekit.space/dopamine/ |
| iDownloadBlog | https://www.idownloadblog.com/2023/05/02/how-to-jailbreak-with-dopamine/ |
| ios.cfw.guide | https://ios.cfw.guide/installing-dopamine-trollstore/ |
| Procursus | https://github.com/ProcursusTeam/Procursus |
| ElleKit | https://github.com/evelynekitty/ElleKit |

**Not found:** opa334 academic paper; Black Hat “Dopamine” talk; full public 2.x primitive write-up.

**Legacy integration docs (purplepois0n):** [LEARNINGS.md](../legacy/LEARNINGS.md) · [REPO_INDEX.md](../legacy/REPO_INDEX.md) · [INTEGRATION_PLAN.md](../legacy/INTEGRATION_PLAN.md) · [COMPARISON_MATRIX.md](../legacy/COMPARISON_MATRIX.md) · [PHASE_STATUS.md](../legacy/PHASE_STATUS.md)
