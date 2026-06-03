# Chapter 5: unc0ver, Electra & Coolstar ecosystem

**Depth TOC:** [L0](#l0--summary) · [L1](#l1--history) · [L2](#l2--ecosystem) · [L3](#l3--security-engineering) · [L4](#l4--host-tooling-architecture) · [L5](#l5--purplepois0n-this-era) · [L6](#l6--sources--further-reading)

## L0 — Summary

2018–2021 split the community between **Pwn20wnd’s unc0ver** (Cydia/Substrate) and **Coolstar’s Electra→Chimera→Odyssey→Taurine** (Substitute/Sileo), all **semi-untethered** on arm64/arm64e with **PAC**, APFS snapshots, and CVE-driven narrow windows.

## L1 — History

| Field | Detail |
|-------|--------|
| **Years** | ~2018–2021 |
| **iOS** | 11.0–14.x (build-dependent); arm64e from A12 |
| **Teams** | Pwn20wnd; Coolstar; many CVE credits (Ian Beer, Brandon Azad, …) |

| Tool | Type | iOS (representative) |
|------|------|----------------------|
| Electra | Semi-untethered | 11.0–11.4.1 |
| unc0ver | Semi-untethered | 11–14.8 (subset) |
| Chimera | Semi-untethered | 12.0–12.5.7 |
| Odyssey / Taurine | Semi-untethered | 13.x / 14.0–14.3 |

Electra EOL Jan 2020 (press). A12 phases sometimes SSH-only before full tweak injection (press/wiki).

## L2 — Ecosystem

| Split | unc0ver side | Coolstar side |
|-------|--------------|---------------|
| Store | Cydia | Sileo (later emphasis) |
| Injection | Substrate | Substitute |
| Snapshots | Less emphasized | Electra APFS snapshot revert (coolstar.org) |
| arm64e | PAC-aware chains | Chimera/Odyssey PAC work |
| User choice | iDownloadBlog comparison articles | Same |

Debate foreshadowed **rootless** (Chapter 7): moving state off writable system volume.

## L3 — Security engineering

**Mitigations**

- **PAC** on A12+ arm64e.
- Hardened **AMFI**, **codesign**, **SEP** boundaries.
- Build-specific CVE windows.

**Chain shape (conceptual)**

1. Multi-bug **userland** (async_wake / voucher_swap generation per Electra credits).
2. **Kernel** exploit with PAC-aware primitives.
3. Trust cache / rootfs remount / entitlements (tool-specific).
4. Package manager + tweak loader; semi-untether app on boot.

Exploit **names only** on coolstar.org credits—no reproduction here.

## L4 — Host tooling architecture

| Function | Typical implementation |
|----------|------------------------|
| Jailbreak app | On-device semi-untether trigger |
| Host USB | Logs, re-sign, AFC file pull |
| Snapshot management | Electra host scripts / APFS APIs (tool-specific) |
| DFU | Not default for these semi tools |

purplepois0n maps host USB to `MobileDevice` + `AFCService`; snapshot/rollback logic is **out of repo**.

## L5 — purplepois0n (this era)

**Branch:** `DeviceState::Normal`.

| Component | Status |
|-----------|--------|
| `performJailbreak()` Normal | **TODO** — semi-untether hook point |
| [`MachOParser`](../../src/MachOParser.h) | **Implemented** — arm64e Mach-O research |
| [`DyldCacheParser`](../../src/DyldCacheParser.h) | **Implemented** — shared cache dumps |
| [`AFCService`](../../src/AFCService.h) | **Implemented** |
| Cydia/Sileo/Substitute packaging | **Out of scope** |

PAC affects **runtime** corruption strategy, not Mach-O on-disk layout—parsers remain valid for static study.

**Deep dive:** [binary-parsers.md](deep/binary-parsers.md)

[GENERATIONS.md — Generation 4](../GENERATIONS.md#generation-4-unc0ver--coolstar-era)

## L6 — Sources & further reading

| Type | URL |
|------|-----|
| Electra | https://coolstar.org/electra/ |
| unc0ver | https://github.com/pwn20wndstuff/Undecimus |
| Electra EOL | https://www.idownloadblog.com/2020/01/04/electra-jailbreak-eol/ |
| Comparison | https://www.idownloadblog.com/2019/05/02/should-i-jailbreak-with-chimera-electra-or-unc0ver/ |
| unc0ver wiki | https://theapplewiki.com/wiki/Unc0ver |

**Not found:** Single Pwn20wnd chain paper; Black Hat Electra exploit deck (individual CVE talks exist separately).

**Legacy integration docs (purplepois0n):** [LEARNINGS.md](../legacy/LEARNINGS.md) · [REPO_INDEX.md](../legacy/REPO_INDEX.md) · [INTEGRATION_PLAN.md](../legacy/INTEGRATION_PLAN.md) · [COMPARISON_MATRIX.md](../legacy/COMPARISON_MATRIX.md) · [PHASE_STATUS.md](../legacy/PHASE_STATUS.md)
