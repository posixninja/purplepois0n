# Legacy download coverage

**Snapshot:** 2026-06-01  
**Policy:** Bulk `git clone` of three GitHub sources only ([`legacy/README.md`](../../legacy/README.md), [`docs/ARCHIVES.md`](../ARCHIVES.md)).

---

## What we HAVE

| Mirror | Path | Upstream | Count (verified) |
|--------|------|----------|------------------|
| Chronic-Dev | `legacy/Chronic-Dev/` | github.com/Chronic-Dev | **36** |
| OpenJailbreak | `legacy/OpenJailbreak/` | github.com/OpenJailbreak | **46** |
| posixninja | `legacy/posixninja/` | github.com/posixninja | **29** |
| **Total** | | | **111** |

### Representative jailbreak *tools* with source in clones

| Era (GENERATIONS.md) | In mirrors | Notes |
|----------------------|------------|-------|
| **Gen 0** | greenpois0n (meta), gp2, gprc5, absinthe, absinthe-2.0, syringe, doctors, poison-jb, medicine, cyanide, anthrax, apparition, libirecovery, idevicerestore, irecovery | Primary lineage; `greenpois0n/` submodules **empty** — use standalone clones |
| **Gen 0 adjacent** | spirit, p0sixspwn, purplesn0w, JailbreakMe-1.0 (OpenJailbreak) | Partial / historical |
| **Gen 1** | evasi0n6, p0sixspwn (OpenJailbreak) | No evad3rs org mirror |
| **Gen 3** | yalu, yalu102 (OpenJailbreak) | Luca-era semi-untether reference |
| **Gen 4** | Undecimus (OpenJailbreak) | Chimera/Odyssey/Taurine/Electra/unc0ver **not** in mirrors |
| **Gen 5** | ipwndfu (OpenJailbreak) | checkm8 reference only; **not** checkra1n/palera1n repos |
| **Gen 6** | — | Dopamine / rootless **not** mirrored |
| **Libraries** | libmbdb, libmacho, libdyldcache, libimg3, libimobiledevice, libusbmuxd, libtss, libipsw, … | Cross-era parser/USB stack |

### Host stack coverage (purplepois0n Gen 0)

P0 set from docs is present: syringe, libirecovery, gp2, gprc5, absinthe-2.0, apparition, libmbdb (OpenJailbreak), posixninja libirecovery-2.0 / libmacho / xpwn / spirit-linux.

---

## Known failures and caveats

| Issue | Detail |
|-------|--------|
| **Chimera13** | Listed on OpenJailbreak; **not cloned** — GitHub HTTP **451** / access blocked |
| **greenpois0n submodules** | Meta-repo cloned; anthrax, cyanide, syringe, etc. under meta path may be **empty shells** — standalone repos under `legacy/Chronic-Dev/` are authoritative |
| **`.gitignore`** | `legacy/**` ignored except `legacy/README.md` — clones stay local, not committed |
| **Duplicate archive** | `~/Desktop/Companies/Chronic-Dev/` exists — **curated docs** (team, timeline, jailbreaks), **not** a 36-repo GitHub mirror; independent of `legacy/Chronic-Dev/` |

---

## What we DON'T have (gaps by generation)

| Gen | Representative tools (GENERATIONS.md / book) | In clones? |
|-----|-----------------------------------------------|------------|
| **0** | redsn0w, Corona, limera1n (geohot) | **Book/wiki only** — iPhone Dev Team / geohot orgs not mirrored |
| **1** | evasi0n, evasi0n7 | **Book only** — evad3rs org not mirrored (`evasi0n6` is partial OpenJailbreak artifact) |
| **2** | Pangu, TaiG, PP Jailbreak | **Book only** — PanguTeam / TaiG not mirrored |
| **3** | extra_recipe, Meridian, Saïgon, h3lix, … | Mostly **book only** (yalu/yalu102 in OpenJailbreak) |
| **4** | Electra, unc0ver, Chimera, Odyssey, Taurine, Fugu14 | **Book only** — Undecimus only; **Chimera13 blocked** |
| **5** | checkra1n, palera1n | **Book only** — ipwndfu only (axi0mX lineage via OpenJailbreak) |
| **6** | Dopamine, Dopamine 2, XinaA15 | **Book only** — opa334/Dopamine not mirrored |

### Public repos we *could* add (selective; not bulk-mirrored today)

| Source | Examples | Why |
|--------|----------|-----|
| **evad3rs** | evasi0n, evasi0n7 (if still public) | Gen 1 chains |
| **PanguTeam** | Pangu releases / tooling | Gen 2 |
| **checkra1n** | checkra1n, palera1n | Gen 5 host DFU workflows |
| **opa334** | Dopamine | Gen 6 rootless |
| **Luca Todesco** | yalu forks beyond OpenJailbreak mirrors | Gen 3 |
| **Pwn20wnd** | unc0ver | Gen 4 |
| **Coolstar** | Electra, Chimera (if public; OpenJailbreak Chimera13 blocked) | Gen 4 |
| **geohot** | limera1n | Gen 0 bootrom context |
| **iPhone Dev Team** | redsn0w (if public) | Gen 0–1 utility |
| **libimobiledevice** (upstream) | canonical libirecovery, idevicebackup2 | Compare to Chronic-Dev forks |

Exploit-only trees in the OpenJailbreak mirror are for **study policy** in docs only — not recommended to vendor staging into purplepois0n.

---

## Recommendation

| Goal | Verdict |
|------|---------|
| **Gen 0 analysis** (DFU, absinthe backup path, mbdb, host I/O) | **Sufficient** — no mandatory extra clones |
| **Full GENERATIONS.md / book parity with source** | **Insufficient** — add **targeted** public clones per chapter, not another bulk org dump |
| **Gen 4 Chimera** | **Cannot** clone OpenJailbreak/Chimera13 via public HTTPS (451) |

**Bottom line:** Download policy completed for **three GitHub orgs** (111 repos). That is **not** every historical jailbreak — by design. For purplepois0n’s Gen‑0 + host-framework focus, the current snapshot is the right scope unless you widen era coverage.

---

## See also

- [REPO_INDEX.md](REPO_INDEX.md) · [LEARNINGS.md](LEARNINGS.md) · [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md) · [PHASE_STATUS.md](PHASE_STATUS.md)
- [GENERATIONS.md](../GENERATIONS.md) · [ARCHIVES.md](../ARCHIVES.md)
