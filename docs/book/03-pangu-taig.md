# Chapter 3: Pangu & TaiG

**Depth TOC:** [L0](#l0--summary) · [L1](#l1--history) · [L2](#l2--ecosystem) · [L3](#l3--security-engineering) · [L4](#l4--host-tooling-architecture) · [L5](#l5--purplepois0n-this-era) · [L6](#l6--sources--further-reading)

## L0 — Summary

Pangu and TaiG (~2014–2016) dominated iOS 7–9 jailbreaking with fast releases, conference “internals” talks, and a shift toward **semi-untethered** behavior on later 9.x builds—while iOS 8+ mitigations (team IDs, central dyld cache, KPP) reshaped both exploit economics and host-side binary analysis.

## L1 — History

| Field | Detail |
|-------|--------|
| **Years** | ~2014–2016 |
| **iOS** | Pangu: 7.1–7.1.2, 8.0–8.1, 9.0–9.3.3; TaiG: 8.0–8.4 |
| **Teams** | Pangu Team; TaiG (lead “XN” in 2014 interview) |

| Tool | Typical type | iOS (representative) |
|------|--------------|----------------------|
| Pangu | Untethered | 7.1.x, 8.0–8.1 |
| Pangu9 | Untethered → semi on 9.2–9.3.3 | 9.x |
| TaiG | Untethered | 8.0–8.4 |

Pangu surprised the Western scene after iOS 7; TaiG filled **8.1.3–8.4** after Pangu8’s narrower 8.0–8.1 window.

## L2 — Ecosystem

| Aspect | Change vs evasi0n era |
|--------|----------------------|
| **Geography** | Pangu/TaiG strong in China; PP Jailbreak regional variants |
| **Conferences** | Black Hat / CanSecWest slides educate on injection targets |
| **9.x norm** | Re-run jailbreak **app** after reboot on 9.2–9.3.3 |
| **Cydia** | Still default; research credits Ian Beer et al. in slides |
| **Host tools** | Windows GUIs + IPA sideloading culture grows |

## L3 — Security engineering

**Mitigations**

- Central **dyld shared cache** — cache-aware primitives.
- **KASLR**, tighter sandboxes, **KPP** on arm64.
- **32-bit vs 64-bit** split per release.

**Chain shape (conceptual)**

1. **Userland** (app, photos trick on 9.x per Wikipedia, or entitlement-heavy services on 8.x per Pangu talk).
2. **Kernel** exploit + slide defeat; KPP bypass on 9.x where needed.
3. Bootstrap; semi-untether → user re-runs app after reboot.

## L4 — Host tooling architecture

| Layer | Era usage |
|-------|-----------|
| Normal USB + app | Primary delivery (especially 9.x semi) |
| AFC / installation_proxy | IPA install, logs (conceptual; tools vary) |
| IPSW / cache dumps on host | Conference slides discuss dyld cache layout |
| DFU | Not primary for Pangu9 mainstream |

Open references: Pangu Black Hat PDF (team ID, injection); libimobiledevice for usbmux/AFC parity with purplepois0n.

## L5 — purplepois0n (this era)

**Branch:** `DeviceState::Normal` (semi-untether re-run maps to repeated Normal sessions).

| Component | Status | Notes |
|-----------|--------|-------|
| [`DyldCacheParser`](../../src/DyldCacheParser.cpp) | **Implemented** | Offline cache from IPSW/research dumps |
| [`MachOParser`](../../src/MachOParser.cpp) | **Implemented** | arm64 Mach-O segments/commands |
| [`MobileDevice`](../../src/MobileDevice.h) | **Implemented** | Version gating, app enumeration |
| [`AFCService`](../../src/AFCService.h) | **Implemented** | File transfer |
| `performJailbreak()` Normal | **TODO** | No “re-jailbreak app” launcher in-tree |

Example research loop (not implemented in CLI): extract `dyld_shared_cache_*` from firmware → `DyldCacheParser::findImage` → `MachOParser` on extracted dylib.

**Deep dive:** [binary-parsers.md](deep/binary-parsers.md)

[GENERATIONS.md — Generation 2](../GENERATIONS.md#generation-2-pangu--taig-era)

## L6 — Sources & further reading

| Type | URL |
|------|-----|
| Pangu site | https://en.pangu.io/ |
| Black Hat 2016 PDF | https://blackhat.com/docs/us-16/materials/us-16-Wang-Pangu-9-Internals.pdf |
| Pangu blog | http://blog.pangu.io/race_condition_bug_92/ |
| TaiG wiki | https://theapplewiki.com/wiki/TaiG |
| Archived taig.com | https://web.archive.org/web/20161201190108/http://www.taig.com/en/ |

**Not found:** Full public Pangu9 9.3.3 chain source; PP Assistant partnership postmortem beyond Wikipedia.

**Legacy integration docs (purplepois0n):** [LEARNINGS.md](../legacy/LEARNINGS.md) · [REPO_INDEX.md](../legacy/REPO_INDEX.md) · [INTEGRATION_PLAN.md](../legacy/INTEGRATION_PLAN.md) · [COMPARISON_MATRIX.md](../legacy/COMPARISON_MATRIX.md) · [PHASE_STATUS.md](../legacy/PHASE_STATUS.md)
