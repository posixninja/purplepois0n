# purplepois0n book: jailbreak eras

> **Educational use only.** This book documents public jailbreak history, security *concepts*, and purplepois0n’s host-side framework. It does **not** teach exploit reproduction, weaponized backups, or bypass of passcode/SEP/activation lock. For legal context, see the [Legal Notice](../../README.md#legal-notice) in the project README.

Educational notes on representative public jailbreak projects from the Chronic Dev Team generation through modern rootless tooling. These chapters complement [GENERATIONS.md](../GENERATIONS.md) and [LINEAGE.md](../LINEAGE.md).

**Audience:** Engineers and researchers studying iOS security history, host-side jailbreak tooling, and how purplepois0n maps to classic workflows.

## Depth ladder

Every chapter uses the same **[L0–L6 sections](DEPTH.md)** so you can skim summaries or drill into host architecture and source code.

| Level | What you get |
|-------|----------------|
| L0 | One-paragraph era summary |
| L1 | History (dates, teams, devices) |
| L2 | Ecosystem (bootstrap, package managers, community shifts) |
| L3 | Security engineering (mitigations, conceptual chain—no steps) |
| L4 | Host tooling architecture (USB, DFU, backup, AFC) |
| L5 | purplepois0n classes + `performJailbreak()` mapping |
| L6 | Sources, “not found”, archive.org hints |

**Start here:** [DEPTH.md](DEPTH.md) — reading paths, TOC markers, disclaimers.

### Deep dives (`deep/`)

Extended **L5** tours with mermaid diagrams and line-referenced source:

| File | Topic |
|------|--------|
| [deep/primitives-gen0.md](deep/primitives-gen0.md) | Primitive taxonomy, `ChainRunner`, `Gen0Workflow` |
| [deep/device-manager.md](deep/device-manager.md) | `DeviceManager` |
| [deep/dfu-recovery.md](deep/dfu-recovery.md) | `DFUDevice`, `RecoveryDevice`, `IRecvUtil` |
| [deep/normal-mode-afc-backup.md](deep/normal-mode-afc-backup.md) | `MobileDevice`, `AFCService`, `MobileBackup` |
| [deep/binary-parsers.md](deep/binary-parsers.md) | `DyldCacheParser`, `MachOParser` |

## Reading order

| Order | Chapter | Era |
|-------|---------|-----|
| 0 | [00-chronic-dev-greenpois0n.md](00-chronic-dev-greenpois0n.md) | greenpois0n, limera1n, DFU (~2010–2011) |
| 1 | [01-chronic-dev-absinthe.md](01-chronic-dev-absinthe.md) | Absinthe, Corona, Rocky Racoon (~2012) |
| 2 | [02-evasi0n-evad3rs.md](02-evasi0n-evad3rs.md) | evasi0n / evasi0n7 (~2013–2014) |
| 3 | [03-pangu-taig.md](03-pangu-taig.md) | Pangu, TaiG (~2014–2016) |
| 4 | [04-yalu-ios10.md](04-yalu-ios10.md) | yalu, Meridian, iOS 10 semi-untether (~2016–2018) |
| 5 | [05-unc0ver-electra-coolstar.md](05-unc0ver-electra-coolstar.md) | Electra, unc0ver, Chimera, Odyssey, Taurine (~2018–2021) |
| 6 | [06-checkra1n-palera1n.md](06-checkra1n-palera1n.md) | checkm8, checkra1n, palera1n (~2019–present) |
| 7 | [07-dopamine-rootless.md](07-dopamine-rootless.md) | Dopamine, rootless (~2021–present) |
| A | [appendix-32bit-legacy.md](appendix-32bit-legacy.md) | Phoenix, Home Depot, 32-bit overlap (optional) |

**Suggested path for newcomers**

1. [LINEAGE.md](../LINEAGE.md) — why purplepois0n exists.
2. [DEPTH.md](DEPTH.md) — how to read at L0 vs L5.
3. [GENERATIONS.md](../GENERATIONS.md) — mitigation timeline and mapping tables.
4. This book — per-era depth.
5. [../../README.md](../../README.md) — build, CLI, `performJailbreak()` development hooks.

## Sources disclaimer

- Chapters cite **public** URLs (research Jan–Jun 2026). Links rot; L6 sections note archive.org where helpful.
- Coverage is **representative**, not exhaustive.
- **Not found** means no strong primary source identified during research—not that nothing exists offline.
- Wiki pages are community-maintained; cross-check dates against Apple release notes when precision matters.

## Quick link

Full generation tables: **[GENERATIONS.md](../GENERATIONS.md)**.
