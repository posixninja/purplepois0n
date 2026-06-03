# Appendix: 32-bit legacy devices

**Depth TOC:** [L0](#l0--summary) · [L1](#l1--history) · [L2](#l2--ecosystem) · [L3](#l3--security-engineering) · [L4](#l4--host-tooling-architecture) · [L5](#l5--purplepois0n-this-era) · [L6](#l6--sources--further-reading)

Optional chapter for hardware on **iOS 9.3.5 / 9.3.6** or **32-bit iOS 10**. Overlaps [GENERATIONS.md — Appendix A](../GENERATIONS.md#appendix-a-32-bit-and-legacy-devices).

## L0 — Summary

Frozen 32-bit hardware (~2016–2019 tool releases) used the same **semi-untether IPA** pattern as 64-bit iOS 10—Phoenix on 9.3.5, h3lix on 10.x—with no Apple security updates and Mach-O **ARM** (not ARM64) binaries for host-side analysis.

## L1 — History

| Field | Detail |
|-------|--------|
| **Years** | ~2016–2019 active releases |
| **Devices** | A5/A6 iPhone 4s, 5/5c, iPad 2–4, iPod touch 5g, … |
| **iOS** | 9.3.5, 9.3.6; 10.x 32-bit only |

When 9.3.5 was the last signed OTA, **Phoenix** (2017) delivered long-awaited semi-untether. **Home Depot** preceded for some 9.x subsets. **h3lix** / **doubleH3lix** covered 32-bit iOS 10 while 64-bit users used yalu (Chapter 4).

| Tool | Type |
|------|------|
| Phoenix | Semi-untethered 9.3.5–9.3.6 |
| Home Depot | Semi-untethered 9.x subsets |
| h3lix / doubleH3lix | Semi-untethered 10.x 32-bit |
| etasonJB | Semi-untethered 8.4.1 32-bit (adjacent) |

## L2 — Ecosystem

| Aspect | 32-bit legacy |
|--------|----------------|
| **Updates** | Frozen firmware—no iOS 11 path |
| **Cydia** | Still primary; smaller tweak corpus |
| **Distribution** | phoenixpwn.com IPA + sideload |
| **Credits** | Siguza + tihmstar (wiki); community drama documented on wiki |
| **vs 64-bit** | No PAC; also no patches from Apple |

## L3 — Security engineering

- No **arm64e PAC**, but **no vendor security fixes**.
- Semi-untether: re-trigger app after reboot.
- **Chain (conceptual):** userland/sandbox escape → kernel patch → Cydia; no modern untether.

## L4 — Host tooling architecture

Same as iOS 10 semi era (Chapter 4): **on-device IPA** primary, host USB for install/logs via usbmux. Phoenix site published IPA SHA-256 and device offsets (public metadata—not exploit steps).

No DFU bootrom lane required for mainstream Phoenix/h3lix.

## L5 — purplepois0n (this era)

**Branch:** `DeviceState::Normal`.

| Component | Status | 32-bit note |
|-----------|--------|-------------|
| [`MachOParser`](../../src/MachOParser.h) | **Implemented** | `MachOCpuType::ARM` for 32-bit Mach-O |
| [`DyldCacheParser`](../../src/DyldCacheParser.h) | **Implemented** | Primarily 64-bit cache layouts in practice—validate `isValid()` on 32-bit dumps |
| `performJailbreak()` | **TODO** | No `DeviceState` arch fork—gate in exploit module via `getCpuType()` |
| [`MobileDevice`](../../src/MobileDevice.h) | **Implemented** | Same lockdown path |

Contributors should branch on CPU type in exploit code outside core framework.

**Deep dive:** [binary-parsers.md](deep/binary-parsers.md)

## L6 — Sources & further reading

| Type | URL |
|------|-----|
| Phoenix wiki | https://www.theiphonewiki.com/wiki/Phoenix |
| Phoenix site | https://phoenixpwn.com/ |
| ios.cfw.guide | https://ios.cfw.guide/installing-phoenix/ |
| Home Depot wiki | https://www.theiphonewiki.com/wiki/Home_Depot |
| h3lix wiki | https://www.theiphonewiki.com/wiki/H3lix |

**Not found:** Formal Phoenix source on major org GitHub (IPA distribution primary).

- [04-yalu-ios10.md](04-yalu-ios10.md)
