# Local GitHub archives

purplepois0n keeps **optional local mirrors** of historical jailbreak-era GitHub organizations under [`../legacy/`](../legacy/). These trees are **not** tracked in git (see root `.gitignore`).

## Sources

| Mirror path | GitHub |
|-------------|--------|
| `legacy/Chronic-Dev/` | [github.com/Chronic-Dev](https://github.com/Chronic-Dev) |
| `legacy/OpenJailbreak/` | [github.com/OpenJailbreak](https://github.com/OpenJailbreak) |
| `legacy/posixninja/` | [github.com/posixninja](https://github.com/posixninja) |

## Integration policy

- **Read-only reference:** Use clones for lineage research, diffing against purplepois0n, and mapping names in [GENERATIONS.md](GENERATIONS.md) / [LINEAGE.md](LINEAGE.md).
- **Do not vendor:** Do not copy entire upstream trees into `src/` or commit archive contents into this repo.
- **Prefer submodule or docs link** when a specific upstream repo must be cited in build docs; keep bulk mirrors in `legacy/` only.
- **Refresh:** Follow commands in [`legacy/README.md`](../legacy/README.md). Re-run list + `git clone` / `git pull` as needed.

## Legacy analysis (purplepois0n evolution)

Structured learnings from the local mirrors live under **`docs/legacy/`**:

| Document | Purpose |
|----------|---------|
| [legacy/LEARNINGS.md](legacy/LEARNINGS.md) | Architecture synthesis per repo/category — data flows, gaps vs `src/`, safe-to-port vs do-not-port |
| [legacy/INTEGRATION_PLAN.md](legacy/INTEGRATION_PLAN.md) | Phased roadmap (host I/O, mbdb, Gen0 hooks, checkm8, book updates) |
| [legacy/REPO_INDEX.md](legacy/REPO_INDEX.md) | Compact repo catalog with study priorities (P0–P3) |
| [legacy/COMPARISON_MATRIX.md](legacy/COMPARISON_MATRIX.md) | Capability grid: greenpois0n vs absinthe vs purplepois0n |
| [legacy/DOWNLOAD_COVERAGE.md](legacy/DOWNLOAD_COVERAGE.md) | Clone coverage by jailbreak generation |
| [legacy/PHASE_STATUS.md](legacy/PHASE_STATUS.md) | Integration phase rollup (Phases 0–5) |

**Known clone gap:** `OpenJailbreak/Chimera13` is not in the snapshot (GitHub HTTP 451). **`Chronic-Dev/greenpois0n`** submodules may be empty unless initialized — use standalone clones (e.g. `legacy/Chronic-Dev/syringe`) per [LEARNINGS.md](legacy/LEARNINGS.md).

## Optional duplicate

You may already have a separate Chronic-Dev tree elsewhere (e.g. `~/Desktop/Companies/Chronic-Dev/`). That copy is independent; `legacy/Chronic-Dev/` exists for a self-contained purplepois0n workspace layout.
