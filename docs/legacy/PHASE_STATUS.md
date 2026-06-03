# Integration phase status

Living rollup for [`INTEGRATION_PLAN.md`](INTEGRATION_PLAN.md). Update this file when a phase lands or acceptance criteria change. Capability truth for users remains [`../SUPPORT.md`](../SUPPORT.md).

**Last reviewed:** 2026-06-03 (Phase 2 closed; ipswd integration landed)

## Summary

| Phase | Name | Status | Notes |
|-------|------|--------|-------|
| **0** | Documentation & reference linking | **Complete** (maintained) | This doc set; refresh when `src/` changes |
| **1** | Host I/O parity | **Complete** | `IRECV_PROGRESS` + `DFUDevice::sendFile` |
| **2** | Backup / Mach-O / mbdb | **Complete** | v1/v2 backup + ipswd/ipsw opaque handles; encrypted decrypt **deferred** |
| **3** | Gen0 workflow hooks | **Complete** | ChainRunner, probes, `--report` |
| **4** | checkm8 (external) | **~90%** | Code + docs done; optional hardware smoke only |
| **5** | Book L5 updates | **Complete** | Ch. 0–1 + deep docs; L6 legacy links in Ch. 2–7 |

## Phase 2 — closed (2026-06-03)

All integration tasks landed. **Deferred by design:** encrypted backup decrypt (keybag / Manifest key). Binary analysis uses **ipswd-first** opaque handles (`IpswdClient`, `MachOBinary`, `DyldSharedCache`).

## Phase 4 — remaining (~10%)

| Item | Status |
|------|--------|
| Physical DFU / Recovery / `-m` checkm8 smoke | Optional — commands in SUPPORT.md |
| External tool version-pin CI | Optional — manual when gaster/ipwndfu updates |

## Phase 0 — Documentation (maintained)

| Task | Status |
|------|--------|
| `legacy/README.md` clone policy + counts | Done |
| `docs/ARCHIVES.md` integration policy | Done |
| `docs/legacy/LEARNINGS.md` | Done — refresh Category 5 when libirecovery API shifts |
| `docs/legacy/REPO_INDEX.md` | Done — component → legacy lookup table |
| `docs/legacy/INTEGRATION_PLAN.md` | Done |
| `docs/legacy/COMPARISON_MATRIX.md` | Done |
| `docs/legacy/DOWNLOAD_COVERAGE.md` | Done |
| Cross-links (`docs/README.md`, book Ch. 0/1, `ARCHIVES.md`, `DEPTH.md`) | Done |
| `docs/book/deep/*` aligned with `src/` | **Ongoing** — see deep dive refresh below |

### Phase 0 acceptance (current)

- Contributor can find **which legacy repo to read** for DFU, backup, or Mach-O via [REPO_INDEX.md](REPO_INDEX.md) without blind cloning.
- **greenpois0n empty submodule** caveat documented in [LEARNINGS.md](LEARNINGS.md) and [ARCHIVES.md](../ARCHIVES.md).
- **SUPPORT.md** and **COMPARISON_MATRIX** reflect shipped primitive/host I/O work (not exploit rows).

## Quick lookup: `src/` → docs

| Code | Primary doc |
|------|-------------|
| `src/DeviceManager.*`, `src/IRecvUtil.*` | [book/deep/device-manager.md](../book/deep/device-manager.md), [book/deep/dfu-recovery.md](../book/deep/dfu-recovery.md) |
| `src/DFUDevice.*`, `src/IRecvProgress.*`, `src/RecoveryDevice.*` | [book/deep/dfu-recovery.md](../book/deep/dfu-recovery.md) |
| `include/primitives/`, `src/primitives/` | [book/deep/primitives-gen0.md](../book/deep/primitives-gen0.md) |
| `src/Gen0Workflow.*` | [SUPPORT.md](../SUPPORT.md), [book/deep/primitives-gen0.md](../book/deep/primitives-gen0.md) |
| `src/MobileBackup.*`, `src/BackupProtocol.*`, `src/MbdbParser.*`, `src/ManifestDbParser.*` | [book/deep/normal-mode-afc-backup.md](../book/deep/normal-mode-afc-backup.md) |
| `src/KeyedArchiverPlist.*` | [book/deep/normal-mode-afc-backup.md](../book/deep/normal-mode-afc-backup.md) |
| `src/MachOBinary.*`, `src/DyldSharedCache.*`, `src/IpswdClient.*`, `src/MachOParser.*`, `src/DyldCacheParser.*` | [book/deep/binary-parsers.md](../book/deep/binary-parsers.md), [BOOGERAIDS.md](../BOOGERAIDS.md) |
| `src/Checkm8.*` | [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md) Phase 4, book Ch. 6 |

## Maintenance checklist (Phase 0)

When changing host I/O or Gen0 workflow:

1. Update [SUPPORT.md](../SUPPORT.md) capability rows if user-visible behavior changes.
2. Update [COMPARISON_MATRIX.md](COMPARISON_MATRIX.md) purplepois0n column.
3. Update relevant `docs/book/deep/*.md` if class APIs or data flows changed.
4. Bump **Last reviewed** date in this file.
5. Mark tasks in [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md) when a phase completes.

## See also

- [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md) — full task lists and acceptance criteria
- [LEARNINGS.md](LEARNINGS.md) — legacy architecture synthesis
- [REPO_INDEX.md](REPO_INDEX.md) — legacy repo catalog
