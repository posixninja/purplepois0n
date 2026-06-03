# Depth ladder — how to read the book

This book is organized in **seven depth levels** (L0–L6). Every chapter uses the same heading names so you can skim one era or drill into host architecture and purplepois0n source without rereading everything.

**Educational only:** L3 describes mitigations and conceptual chain *stages*, not exploit reproduction. L5 documents framework APIs and `performJailbreak()` hooks, not weaponized payloads.

## Levels

| Level | Heading in chapters | Purpose | Typical reader |
|-------|---------------------|---------|----------------|
| **L0** | `## L0 — Summary` | One-paragraph era snapshot | Skimmers, index users |
| **L1** | `## L1 — History` | Dates, teams, jailbreak type, devices/iOS | Historians, timelines |
| **L2** | `## L2 — Ecosystem` | Bootstrap, package managers, community shifts vs prior era | Jailbreak users studying *culture* of each generation |
| **L3** | `## L3 — Security engineering` | Mitigations, threat model, conceptual chain stages (no steps) | Security engineers |
| **L4** | `## L4 — Host tooling architecture` | How desktop tools talked to devices (USB, DFU, backup, AFC, apps) | Host-tool developers |
| **L5** | `## L5 — purplepois0n (this era)` | In-tree classes, data flow, implemented vs TODO; links to `docs/book/deep/` | purplepois0n contributors |
| **L6** | `## L6 — Sources & further reading` | Bibliography, “not found”, archive.org hints | Researchers citing sources |

Higher levels **assume** lower levels only when noted (e.g. L5 may say “see L4 for usbmuxd context”).

## TOC markers in chapters

Each chapter begins with a **depth table of contents** — a bullet list linking to `## L0` … `## L6` anchors. In GitHub and most Markdown viewers, those links jump directly to the section.

Section headings are always exactly:

```text
## L0 — Summary
## L1 — History
## L2 — Ecosystem
## L3 — Security engineering
## L4 — Host tooling architecture
## L5 — purplepois0n (this era)
## L6 — Sources & further reading
```

Do not rename these headings if you add content; tools and cross-chapter navigation depend on the stable labels.

## Optional deep dives (L5+)

Files under [`deep/`](deep/) are **extended L5** walkthroughs with mermaid diagrams and line-referenced source tours:

| File | Topic |
|------|--------|
| [deep/primitives-gen0.md](deep/primitives-gen0.md) | Primitive taxonomy, `ChainRunner`, `Gen0Workflow` |
| [deep/device-manager.md](deep/device-manager.md) | `DeviceManager` detection, enumeration, factory methods |
| [deep/dfu-recovery.md](deep/dfu-recovery.md) | `DFUDevice`, `RecoveryDevice`, `IRecvUtil`, irecv USB surface |
| [deep/normal-mode-afc-backup.md](deep/normal-mode-afc-backup.md) | `MobileDevice`, `AFCService`, `MobileBackup` |
| [deep/binary-parsers.md](deep/binary-parsers.md) | `DyldCacheParser`, `MachOParser` |

Read a chapter’s L5 first; open a deep file when you need implementation detail beyond the era summary.

## Suggested reading paths

| Goal | Path |
|------|------|
| Fast timeline | L0 only in each chapter, or [GENERATIONS.md](../GENERATIONS.md) |
| Understand why an era felt different | L1 + L2 |
| Security course / mitigation study | L3 across chapters + GENERATIONS mitigation table |
| Reimplement host I/O (not exploits) | L4 + `deep/` + root [README.md](../../README.md) |
| Extend purplepois0n | L5 in target era + all of `deep/` + [legacy/PHASE_STATUS.md](../legacy/PHASE_STATUS.md) |
| Study legacy sources before porting | [legacy/REPO_INDEX.md](../legacy/REPO_INDEX.md) + [legacy/LEARNINGS.md](../legacy/LEARNINGS.md) |

## Relationship to other docs

| Document | Role |
|----------|------|
| [LINEAGE.md](../LINEAGE.md) | Why purplepois0n exists (pre-L0 narrative) |
| [GENERATIONS.md](../GENERATIONS.md) | Tabular generation reference (often L1+L3 compressed) |
| [book/README.md](README.md) | Chapter order and disclaimers |
| Root [README.md](../../README.md) | Build, CLI, legal notice |
| [legacy/PHASE_STATUS.md](../legacy/PHASE_STATUS.md) | Integration phases 0–5 rollup |
| [legacy/REPO_INDEX.md](../legacy/REPO_INDEX.md) | Legacy repo catalog |

## What we do not document at any level

- Exploit reproduction, PoC bytes, or timing-dependent trigger sequences
- Weaponized backup restore recipes or malicious profile payloads
- Bypass instructions for passcode, SEP, or Apple activation lock

Those belong in responsible-disclosure channels and vendor fixes—not in this book.
