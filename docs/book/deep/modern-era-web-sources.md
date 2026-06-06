# Modern era — web source catalog (PUAF / Dopamine / rootless)

**Purpose:** Curated bibliography of **public web sources** for Generation 6 (rootless, PUAF/kfd, Dopamine 2.x). Use alongside [puaf-kfd-era.md](puaf-kfd-era.md) and [Chapter 7](../07-dopamine-rootless.md).

**Last collected:** 2026-06-03 (gaps pass: dmaFail, DarkSword CVEs, Nullcon talk, support matrix).

**Educational only** — indexes papers, repos, advisories, and analysis; no exploit steps.

---

## How to use this catalog

| If you need… | Start here |
|--------------|------------|
| PUAF vocabulary + pipeline | [felix-pb/kfd README](https://github.com/felix-pb/kfd) → write-ups below |
| Per-bug PUAF deep dives | [writeups/](https://github.com/felix-pb/kfd/tree/main/writeups) on felix-pb/kfd |
| Dopamine exploit modules in-tree | [opa334/Dopamine `Application/Dopamine/Exploits/`](https://github.com/opa334/Dopamine/tree/2.x/Application/Dopamine/Exploits) |
| Supported iOS ranges (changes often) | [Dopamine releases](https://github.com/opa334/Dopamine/releases) — **not** stale README alone |
| Install architecture (not exploits) | [ios.cfw.guide — Dopamine + TrollStore](https://ios.cfw.guide/installing-dopamine-trollstore/) |

---

## 1. PUAF / kfd / libkfd

### Primary repositories

| Repo | Role | URL |
|------|------|-----|
| **felix-pb/kfd** | Original libkfd + write-ups | https://github.com/felix-pb/kfd |
| **opa334/kfd** | Fork used by Dopamine integration | https://github.com/opa334/kfd |
| **wh1te4ever/kfund** | Community fork (subset of PUAF methods in some builds) | https://github.com/wh1te4ever/kfund |

### libkfd API (header)

| Resource | URL |
|----------|-----|
| `libkfd.h` — `puaf_method`, `kopen`, `kread`, `kwrite` | https://github.com/felix-pb/kfd/blob/main/kfd/libkfd.h |

### Write-ups (felix-pb/kfd)

| Doc | Topic | URL |
|-----|-------|-----|
| **Exploiting PUAFs** | Generic KRKW after dangling PTEs | https://github.com/felix-pb/kfd/blob/main/writeups/exploiting-puafs.md |
| **PhysPuppet** | CVE-2023-23536; SockPuppet lineage | https://github.com/felix-pb/kfd/blob/main/writeups/physpuppet.md |
| **Smith** | CVE-2023-32434 | https://github.com/felix-pb/kfd/blob/main/writeups/smith.md |
| **Landa** | CVE-2023-41974 | https://github.com/felix-pb/kfd/blob/main/writeups/landa.md |
| Write-ups index | All figures + PDF assets | https://github.com/felix-pb/kfd/tree/main/writeups |

### PUAF methods ↔ Apple advisories

| libkfd method | CVE | Apple security content | Fixed (representative) |
|---------------|-----|------------------------|-------------------------|
| `puaf_physpuppet` | CVE-2023-23536 | https://support.apple.com/en-us/HT213676 | iOS 16.4, macOS 13.3 |
| `puaf_smith` | CVE-2023-32434 | https://support.apple.com/en-us/HT213814 | iOS 16.5.1, macOS 13.4.1 |
| `puaf_landa` | CVE-2023-41974 | https://support.apple.com/en-us/HT213938 | iOS 17.0, macOS 14.0 |

**Public notes (kfd README):** PhysPuppet reachable from **App Sandbox**, not WebContent; Smith reachable from **WebContent** (ITW reports cited upstream); Landa App Sandbox, not WebContent. Bounty amounts listed in felix-pb README ($52.5k / Smith uncited / $70k).

### KRKW method names (API only)

Documented in [Exploiting PUAFs](https://github.com/felix-pb/kfd/blob/main/writeups/exploiting-puafs.md):

| Kind | Enum values |
|------|-------------|
| kread | `kread_kqueue_workloop_ctl`, `kread_sem_open` |
| kwrite | `kwrite_dup`, `kwrite_sem_open` |

### Related inspiration (cited in PhysPuppet write-up)

| Name | Note | URL |
|------|------|-----|
| SockPuppet | Prior dangling-PTE lineage (Ned Williamson) | Search: Project Zero / SockPuppet XNU (not duplicated here) |

---

## 2. Dopamine 1.x — Fugu15 lineage (arm64e)

### History & talks

| Resource | URL |
|----------|-----|
| **Fugu15** — Linus Henze, OBTS v5 slides | https://objectivebythesea.org/v5/talks/OBTS_v5_lHenze.pdf |
| Fugu15 wiki (exploit list + CVEs) | https://theapplewiki.com/wiki/Fugu15 |
| Fugu15 on AppleDB | https://appledb.dev/jailbreak/Fugu15.html |
| opa334 **Fugu15** releases (archived betas) | https://github.com/opa334/Fugu15/releases |
| Pinauten / LinusHenze GitHub | https://github.com/LinusHenze |

### Fugu15 / Dopamine 1.x kernel & bypass modules (public CVE mapping)

Sources: [Fugu15 wiki](https://theapplewiki.com/wiki/Fugu15), [Dopamine wiki — Exploits 1.x](https://theapplewiki.com/wiki/Dopamine), OBTS slides.

| Module | Role | CVE (public) |
|--------|------|--------------|
| **oobPCI** | Kernel R/W | CVE-2022-26763 |
| **CoreTrust** / fastPath | Codesign / trust cache path | CVE-2022-26766 |
| **badRecovery** | PAC / CFI bypass | CVE-2022-26765 |
| **tlbFail** | PPL bypass | CVE-2022-26764 |

---

## 2.5 dmaFail — PPL bypass (Dopamine 2.x)

**Role:** **Page Protection Layer (PPL) bypass** used alongside kernel R/W (e.g. kfd) on Dopamine 2.x — not a kernel read/write exploit by itself. Publicly tied to **Operation Triangulation** (Kaspersky).

| Resource | URL |
|----------|-----|
| Dopamine in-tree module | https://github.com/opa334/Dopamine/blob/2.x/Application/Dopamine/Exploits/dmaFail/dmaFail.c |
| **dmaFail** wiki | https://theapplewiki.com/wiki/DmaFail |
| **CVE-2023-38606** — iOS 16.6 security content | https://support.apple.com/en-us/120338 |
| Same CVE — macOS Monterey 12.6.8 | https://support.apple.com/en-us/HT213844 |
| Kaspersky — hardware MMIO / “last mystery” | https://securelist.com/operation-triangulation-the-last-hardware-mystery/111669/ |
| Kaspersky press (37C3 disclosure) | https://usa.kaspersky.com/about/press-releases/kaspersky-discloses-iphone-hardware-feature-vital-in-operation-triangulation-case |
| **37C3 talk** — Operation Triangulation | https://www.youtube.com/watch?v=1f6YyH62jFE |
| Wikipedia — Operation Triangulation | https://en.wikipedia.org/wiki/Operation_Triangulation |
| BleepingComputer summary | https://www.bleepingcomputer.com/news/security/iphone-triangulation-attack-abused-undocumented-hardware-feature/ |

**Public mechanism (conceptual):** Undocumented **AGX coprocessor (GPU) MMIO / debug registers** let attacker-controlled cache lines flush to **physical DRAM**, including PPL-protected regions → PTE manipulation → stable PPL bypass ([wiki](https://theapplewiki.com/wiki/DmaFail), [Securelist](https://securelist.com/operation-triangulation-the-last-hardware-mystery/111669/)).

**Patch / reach (public):** Fixed **iOS 16.6** (CVE-2023-38606). Wiki notes **A15/A16 on iOS 16.5.1** may be unexploitable (extra debug register disabled). Dopamine `dmaFail.c` gates on `hw.cpufamily` for A12–A16.

**Overlap with kfd:** Triangulation also used **CVE-2023-32434** — the same bug as libkfd **`puaf_smith`** (§1).

---

## 3. Dopamine 2.x — kernel exploit picker

### Canonical project

| Resource | URL |
|----------|-----|
| **Dopamine** (branch `2.x`) | https://github.com/opa334/Dopamine |
| Exploit frameworks directory | https://github.com/opa334/Dopamine/tree/2.x/Application/Dopamine/Exploits |
| **XPF** patchfinder | https://github.com/opa334/XPF |
| Release notes (support truth) | https://github.com/opa334/Dopamine/releases |

### Module ↔ upstream (integrated in Dopamine)

| Dopamine module | Upstream / author | Primary public source | CVE / bug (where known) |
|-----------------|-------------------|----------------------|-------------------------|
| **kfd** | felix-pb / opa334 | https://github.com/opa334/kfd | See §1 (PUAF CVEs) |
| **dmaFail** | Operation Triangulation / Kaspersky | https://github.com/opa334/Dopamine/tree/2.x/Application/Dopamine/Exploits/dmaFail | **CVE-2023-38606** — PPL bypass (§2.5) |
| **weightBufs** | @_simo36 (Simone Ferrini) | https://github.com/0x36/weightBufs | CVE-2022-32845, -32948, -42805, -32899 (ANE); [POC slides PDF](https://github.com/0x36/weightBufs/blob/main/attacking_ane_poc2022.pdf) |
| **multicast_bytecopy** | John Åkerblom (@jaakerblom) | https://github.com/potmdehex/multicast_bytecopy | CVE-2021-30937; [Project Zero #2224](https://bugs.chromium.org/p/project-zero/issues/detail?id=2224) |
| **multicast_bytecopy_A9** (fork) | wh1te4ever | https://github.com/wh1te4ever/multicast_bytecopy_A9 | A9 device adaptations |
| **badRecovery** | Fugu15 lineage | In Dopamine tree | CVE-2022-26765 — PAC bypass (legacy arm64e path) |
| **DarkSword** | ITW chain → jailbreak port | https://github.com/opa334/darksword-kexploit | Kernel R/W stage **CVE-2025-43520** (ITW); port uses ICMPv6 technique — §4 |

**Picker roles (conceptual):** **kfd** / **weightBufs** / **multicast_bytecopy** / **DarkSword** → kernel R/W; **dmaFail** → PPL bypass where still applicable; **badRecovery** → PAC on Fugu15-era paths. Exact pairing is build- and SoC-specific inside Dopamine.

**Community wiki changelog** (release-level detail): https://theapplewiki.com/wiki/Dopamine — search “weightBufs”, “DarkSword”, “kfd”, “2.0”, “2.5”.

**Example release (DarkSword landing):** https://github.com/opa334/Dopamine/commit/9dc2267396ca8db3b3d9f88e6c525b8172ae9c0d

### 3.1 Dopamine support matrix (release-sourced)

**Rule:** Treat [GitHub Releases](https://github.com/opa334/Dopamine/releases) + [Apple Wiki — Dopamine](https://theapplewiki.com/wiki/Dopamine) as authoritative. README on default branch **lags** (still lists ~16.6.1 arm64 as of 2.4.x).

| Release | Published | arm64e (public notes) | arm64 (public notes) | Source |
|---------|-----------|------------------------|----------------------|--------|
| **2.4.8** | 2026-03-14 | iOS 15.0–16.5.1 (README baseline) | iOS 15.0–15.8.6 / 16.0–16.6.1 (README baseline) | [2.4.8](https://github.com/opa334/Dopamine/releases/tag/2.4.8) · README |
| **2.4.9** | 2026-04-25 | Same as 2.4.x line | Same | [2.4.9](https://github.com/opa334/Dopamine/releases/tag/2.4.9) |
| **2.5b3** | 2026-05-13 | (unchanged in release notes) | **+ iOS 16.7.16, 17.0–18.7.1** via **DarkSword** (beta) | [2.5b3](https://github.com/opa334/Dopamine/releases/tag/2.5b3) |

**Community wiki rollup** (device-level nuance — verify against release before relying):

| CPU / class | iOS range (wiki, 2026) | Source |
|-------------|------------------------|--------|
| **arm64** | 15.0–15.8.8, 16.0–16.7.16; 2.5b adds 17–18.7.x (beta) | [Dopamine wiki](https://theapplewiki.com/wiki/Dopamine) |
| **arm64e** | 15.0–15.8.6, 16.0–16.5; A12–A14 **16.5.1** | [Dopamine wiki](https://theapplewiki.com/wiki/Dopamine) |

**2.5b3 known issues (from release):** DarkSword — no A8(X); A9X flaky; iOS 16.0–16.3.1 race failures; 2GB RAM devices on iOS 17+ (e.g. iPad 6). Zebra entitlement crash on some newer iOS versions.

---

## 4. DarkSword (full chain vs kernel-only port)

DarkSword is a **multi-stage exploit kit** (WebContent → sandbox escapes → kernel). Dopamine integrates an **Objective-C kernel exploit port** ([darksword-kexploit](https://github.com/opa334/darksword-kexploit)), not the full browser-delivered chain.

### Full chain CVEs (ITW — six stages)

Sources: [Google GTIG](https://cloud.google.com/blog/topics/threat-intelligence/darksword-ios-exploit-chain), [8kSec stage table](https://8ksec.io/how-browser-exploits-work-darksword-ios-cve-2025-43529/), [Lookout](https://security.lookout.com/threat-intelligence/article/darksword-exploit-kit).

| Stage | CVE | Component | Role (public) | Patched (representative) |
|-------|-----|-----------|---------------|---------------------------|
| 1a | CVE-2025-31277 | JavaScriptCore JIT | WebContent RCE (iOS 18.4–18.5) | iOS 18.6 |
| 1b | CVE-2025-43529 | DFG JIT (0-day) | WebContent RCE (iOS 18.6–18.7) | iOS 18.7.3 / 26.2 |
| 2 | CVE-2026-20700 | dyld | PAC bypass → native calls | iOS 26.3 |
| 3 | CVE-2025-14174 | ANGLE / WebGL | Sandbox escape → GPU | iOS 18.7.3 / 26.2 |
| 4 | CVE-2025-43510 | XNU (CoW) | GPU → `mediaplaybackd` | iOS 18.7.2 / 26.1 |
| 5 | **CVE-2025-43520** | **XNU VFS (race)** | **Kernel arbitrary R/W** | iOS 18.7.2 / 26.1 |

**Dopamine jailbreak relevance:** Picker module targets the **kernel R/W stage technique** (ICMPv6 `ICMP6_FILTER` / socket PCB manipulation — [matteyeux](https://matteyeux.com/posts/darksword/), [RRR3d/DarkSword §4.1](https://github.com/RRR3d/DarkSword)). ITW stage-5 CVE **CVE-2025-43520** is the documented kernel privilege escalation in the chain; the port runs from the **jailbreak app** with different entry constraints than watering-hole WebContent.

**CISA KEV (2026):** CVE-2025-31277, CVE-2025-43510, CVE-2025-43520 — [Lookout](https://security.lookout.com/threat-intelligence/article/darksword-exploit-kit).

| Resource | Focus | URL |
|----------|-------|-----|
| **Google GTIG** — proliferation / CVE table | ITW surveillance kit | https://cloud.google.com/blog/topics/threat-intelligence/darksword-ios-exploit-chain |
| **matteyeux** (FR) — chain walkthrough | ICMPv6 KRW, IOSurface race | https://matteyeux.com/posts/darksword/ |
| **AntonioCiolino/DarkSword-Analysis** | Reconstructed chain docs | https://github.com/AntonioCiolino/DarkSword-Analysis |
| **RRR3d/DarkSword** | Community reconstruction repo | https://github.com/RRR3d/DarkSword |
| **Mobile Hacking Course** | Six-CVE overview (WebKit → kernel) | https://mobilehackingcourse.com/darksword-inside-the-six-vulnerability-ios-exploit-kit-used-by-state-sponsored-hackers/ |

### Jailbreak-oriented ports

| Repo | URL |
|------|-----|
| **opa334/darksword-kexploit** | https://github.com/opa334/darksword-kexploit |
| Kev1nLevin/darksword-kexploit-ios18 (offset fork) | https://github.com/Kev1nLevin/darksword-kexploit-ios18 |

**Dopamine-known issues (2.5 beta):** no A8(X); A9X flaky; iOS 16.0–16.3.1 race failures; 2GB RAM on iOS 17+ — see [2.5b3 release](https://github.com/opa334/Dopamine/releases/tag/2.5b3).

---

## 5. Bootstrap, injection, install path

| Component | URL |
|-----------|-----|
| **Procursus** bootstrap | https://github.com/ProcursusTeam/Procursus |
| **ElleKit** injection | https://github.com/tealbathingsuit/ellekit |
| **libroot** (rootless paths) | https://github.com/opa334/libroot |
| **libkrw** API (Siguza) | https://github.com/Siguza/libkrw |
| Dopamine vendored libkrw plugin header | https://github.com/opa334/Dopamine/blob/2.x/BaseBin/_external/include/libkrw/libkrw_plugin.h |
| **TrollStore** | https://github.com/opa334/TrollStore |
| **libgrabkernel2** (kernelcache download, betas) | https://github.com/alfiecg24/libgrabkernel2 |
| libgrabkernel (original) | https://github.com/tihmstar/libgrabkernel |
| **roothide** (optional rootless prefix) | https://github.com/RootHide/Developer |

### User-facing install guides (architecture only)

| Guide | URL |
|-------|-----|
| ellekit.space / Dopamine download | https://ellekit.space/dopamine/ |
| ios.cfw.guide — Dopamine + TrollStore | https://ios.cfw.guide/installing-dopamine-trollstore/ |
| iDownloadBlog — Dopamine how-to (2023) | https://www.idownloadblog.com/2023/05/02/how-to-jailbreak-with-dopamine/ |

---

## 6. Press & secondary coverage

| Article | URL |
|---------|-----|
| iDownloadBlog — multicast_bytecopy release (2022) | https://www.idownloadblog.com/2022/04/27/another-ios-15-0-15-1-1-kernel-exploit-released-this-time-with-backward-adaptability-for-newer-versions-of-ios-14/ |
| iDownloadBlog — Dopamine launch | https://www.idownloadblog.com/2023/05/02/how-to-jailbreak-with-dopamine/ |

---

## 7. Version / device databases

| Resource | URL |
|----------|-----|
| **AppleDB** (jailbreak + firmware) | https://appledb.dev/ |
| The Apple Wiki — Dopamine | https://theapplewiki.com/wiki/Dopamine |
| The Apple Wiki — Fugu15 | https://theapplewiki.com/wiki/Fugu15 |

---

## 8. purplepois0n-relevant host research (same era)

| Need | Public tool | purplepois0n |
|------|-------------|--------------|
| Mach-O / dyld from IPSW | [blacktop/ipsw](https://github.com/blacktop/ipsw), ipswd | `MachOBinary`, `DyldSharedCache`, [BOOGERAIDS.md](../../BOOGERAIDS.md) |
| Kernelcache on device | libgrabkernel2 | **Not in-tree** |
| USB AFC / lockdown | libimobiledevice | `AFCService`, `MobileDevice` |

---

## 9. Conference talks & primary analysis (opa334 / Triangulation / PUAF)

| Talk / article | Speaker / org | Topic | URL |
|----------------|---------------|-------|-----|
| **Nullcon Goa 2025** — “State Of iOS Jailbreaking In 2025” | Lars Fröder (@opa334) | Dopamine, TrollStore, PUAF/kfd limits, iOS 17/18 outlook | https://www.youtube.com/watch?v=lU2lxGtLN6k |
| Nullcon coverage | iDownloadBlog | Slide recap, video link | https://www.idownloadblog.com/2025/05/13/dopamine-developer-shares-full-talk-from-nullcon-goa-2025/ |
| Early Nullcon preview | iDownloadBlog | SPTM vs PPL, PUAF patched iOS 17.3 | https://www.idownloadblog.com/2025/03/01/opa334-ios-17-18-jailbreak-eta-nullcon-goa-2025/ |
| **37C3** — Operation Triangulation | Kaspersky GReAT | Spyware chain; hardware MMIO / dmaFail class | https://www.youtube.com/watch?v=1f6YyH62jFE |
| **OBTS v5** — Fugu15 | Linus Henze | oobPCI, badRecovery, tlbFail (Dopamine 1.x lineage) | https://objectivebythesea.org/v5/talks/OBTS_v5_lHenze.pdf |

**Still not found:** Peer-reviewed **academic paper** authored by opa334 on PUAF/Dopamine; conference talk literally titled **“Dopamine”** at Black Hat/DEF CON (Nullcon 2025 covers the same author + project).

---

## 10. Remaining gaps / intentionally omitted

| Topic | Status |
|-------|--------|
| Peer-reviewed opa334 paper on PUAF/Dopamine | **Not found** — use Nullcon 2025 video (§9) |
| Exploit reproduction in purplepois0n | **Out of scope** — [DEPTH.md](../DEPTH.md) |
| Full XPF offset database | **Runtime / XPF repo** — not a static web doc |
| Authoritative **per-device** matrix in one Apple doc | **Use releases + wiki + in-app picker** |

---

## 11. Suggested reading order (researcher)

1. [felix-pb/kfd README](https://github.com/felix-pb/kfd) + [Exploiting PUAFs](https://github.com/felix-pb/kfd/blob/main/writeups/exploiting-puafs.md)
2. [puaf-kfd-era.md](puaf-kfd-era.md) (purplepois0n mapping)
3. [Securelist — Operation Triangulation MMIO](https://securelist.com/operation-triangulation-the-last-hardware-mystery/111669/) → **dmaFail** / PPL (§2.5)
4. [Fugu15 OBTS slides](https://objectivebythesea.org/v5/talks/OBTS_v5_lHenze.pdf) → Dopamine 1.x context
5. [Nullcon Goa 2025 talk](https://www.youtube.com/watch?v=lU2lxGtLN6k) → Dopamine 2 / ecosystem (§9)
6. [Dopamine releases](https://github.com/opa334/Dopamine/releases) + §3.1 support matrix
7. Optional ITW: [Google GTIG DarkSword](https://cloud.google.com/blog/topics/threat-intelligence/darksword-ios-exploit-chain)

---

## Maintenance

Refresh this catalog when:

- Dopamine adds/removes an exploit module (watch `Application/Dopamine/Exploits/` and releases)
- Apple ships security updates affecting listed CVEs
- New public write-up for weightBufs / DarkSword / kfd forks

**See also:** [puaf-kfd-era.md](puaf-kfd-era.md) · [Chapter 7](../07-dopamine-rootless.md) · [GENERATIONS.md](../../GENERATIONS.md#generation-6-rootless-modern-era)
