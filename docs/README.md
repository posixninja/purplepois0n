# purplepois0n documentation

This directory documents the historical lineage of iOS jailbreak tooling—from the Chronic Dev Team era through modern rootless and checkm8-based workflows—and explains how **purplepois0n** fits into that landscape as a research framework.

The material here is **educational**: it catalogs public tool names, iOS version ranges, security mitigations, and conceptual chain shapes. It does not provide exploit recipes, weaponized backup formats, or step-by-step instructions for compromising devices.

## Depth system (book)

The jailbreak **book** uses a shared **[L0–L6 depth ladder](book/DEPTH.md)** in every chapter (summary → history → ecosystem → security engineering → host tooling → purplepois0n source mapping → bibliography). Skim **L0** only for a timeline, or read **L4–L5** with the [`book/deep/`](book/deep/) walkthroughs when implementing host-side framework code.

## Documents

| Document | Description |
|----------|-------------|
| [SUPPORT.md](SUPPORT.md) | **Honest Gen 0 matrix** — what greenpois0n/absinthe did vs what is implemented today |
| [BOOGERAIDS.md](BOOGERAIDS.md) | **boogeraids handoff** — `--analyze-json`, **ipswd** `:3993`, `external/ipsw` fallback |
| [LINEAGE.md](LINEAGE.md) | Predecessors (greenpois0n, absinthe), what changed afterward, and purplepois0n’s role today |
| [GENERATIONS.md](GENERATIONS.md) | Full generation-by-generation reference with mitigations, tool tables, and framework mapping |
| [BACKPORT_MATRIX.md](BACKPORT_MATRIX.md) | **What can be backported** from modern framework work to older iOS eras (host vs exploit) |
| [book/deep/puaf-kfd-era.md](book/deep/puaf-kfd-era.md) | **PUAF / kfd / Dopamine 2** — later-era kernel primitive stack (conceptual) |
| [book/deep/modern-era-web-sources.md](book/deep/modern-era-web-sources.md) | **Web bibliography** — curated public sources for Gen 6 / PUAF era |
| [ARCHIVES.md](ARCHIVES.md) | Local `legacy/` mirror policy and refresh commands |
| [book/DEPTH.md](book/DEPTH.md) | Depth ladder (L0–L6), TOC markers, reading paths |
| [book/README.md](book/README.md) | Educational chapters per jailbreak era + `deep/` source tours |

## Legacy analysis

Read-only clones of Chronic-Dev, OpenJailbreak, and posixninja live under [`../legacy/`](../legacy/) (gitignored). The **`docs/legacy/`** set turns those mirrors into an actionable evolution guide:

| Document | Description |
|----------|-------------|
| [legacy/LEARNINGS.md](legacy/LEARNINGS.md) | Main synthesis — architecture, data flows, gaps, safe-to-port boundaries |
| [legacy/MODERN_ERA_LEARNINGS.md](legacy/MODERN_ERA_LEARNINGS.md) | **Gen 6** mirror synthesis — Dopamine, kfd, XPF, host vs device |
| [legacy/INTEGRATION_PLAN.md](legacy/INTEGRATION_PLAN.md) | Phased roadmap (host I/O, mbdb, Gen0 hooks, checkm8, Phase 6 primitives) |
| [legacy/REPO_INDEX.md](legacy/REPO_INDEX.md) | Repo catalog and study priorities |
| [legacy/COMPARISON_MATRIX.md](legacy/COMPARISON_MATRIX.md) | greenpois0n vs absinthe vs purplepois0n capability grid |
| [BACKPORT_MATRIX.md](BACKPORT_MATRIX.md) | Modern → older generation backport feasibility (host vs exploit) |
| [legacy/DOWNLOAD_COVERAGE.md](legacy/DOWNLOAD_COVERAGE.md) | What is cloned vs missing by jailbreak generation |
| [legacy/PHASE_STATUS.md](legacy/PHASE_STATUS.md) | **Integration phase rollup** — Phases 0–6 and `src/` → doc lookup |

See also [ARCHIVES.md](ARCHIVES.md) for clone policy and known failures (e.g. Chimera13 HTTP 451).

## Suggested reading order

1. **[SUPPORT.md](SUPPORT.md)** — Capability matrix for Generation 0 (DFU, Recovery, backup parse vs restore, exploits).
2. **[LINEAGE.md](LINEAGE.md)** — Start here if you are new to the project. It explains why purplepois0n exists and how it relates to greenpois0n and absinthe.
3. **[GENERATIONS.md](GENERATIONS.md)** — Read when you need era-specific context (which mitigations appeared when, which jailbreak type was normal, which purplepois0n components apply).
4. **[BACKPORT_MATRIX.md](BACKPORT_MATRIX.md)** — What modern host/framework work applies to older eras (Phase 7 roadmap).
5. **[legacy/PHASE_STATUS.md](legacy/PHASE_STATUS.md)** — Integration phase rollup (Phases 0–7) and `src/` → doc lookup.
6. **[book/DEPTH.md](book/DEPTH.md)** — How to read chapters at L0 vs L5; link to `book/deep/` code walkthroughs.
7. **[book/README.md](book/README.md)** — Per-era chapters (L0–L6) with public source links (educational; not exploit recipes).
8. **[../README.md](../README.md)** — Return to the root README for build instructions, CLI usage, and the [Development](../README.md#development) section on adding exploit code in `performJailbreak()`.

## For contributors implementing exploits

When adding code to purplepois0n:

1. Identify the **generation** and **device mode** (Normal, Recovery, DFU) your work targets—see the purplepois0n mapping subsection in each generation of [GENERATIONS.md](GENERATIONS.md).
2. Reuse existing framework classes (`MobileDevice`, `DFUDevice`, `AFCService`, parsers) rather than duplicating host-side I/O.
3. Hook exploits via [`include/primitives/`](../../include/primitives/Primitives.h) registry or [`src/Gen0Workflow.cpp`](../src/Gen0Workflow.cpp) for the matching `DeviceState` branch.
4. Update [SUPPORT.md](SUPPORT.md) if you add new framework surfaces or change what is intentionally unsupported.
5. Update [legacy/COMPARISON_MATRIX.md](legacy/COMPARISON_MATRIX.md), [BACKPORT_MATRIX.md](BACKPORT_MATRIX.md), and [legacy/PHASE_STATUS.md](legacy/PHASE_STATUS.md) when integration phases advance.

## Disclaimer

Jailbreak research may void warranties, weaken device security, and be restricted in some jurisdictions. See the [Legal Notice](../README.md#legal-notice) in the root README.
