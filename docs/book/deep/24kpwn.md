# Deep dive: 24kpwn (0x24000 Segment Overflow)

**Depth:** L5  
**Sources:** [The Apple Wiki — 0x24000 Segment Overflow](https://theapplewiki.com/wiki/0x24000_Segment_Overflow), Chronic Dev / redsn0w era tooling, `include/primitives/HistoricalExploitModules.h`

**24kpwn** (formal name: **0x24000 Segment Overflow**) is a bootROM-class bug on **old-bootrom** **S5L8720** / **iPhone 3GS (old BR)** silicon. It overflows the SHA1 upload path during DFU IMG3 handling so a crafted LLB IMG3 can redirect execution and install an **untethered** jailbreak without re-running the host tool after reboot.

It predates **limera1n** (geohot, Oct 2010) and targets a narrower device set. On those models it often **paired with limera1n**: limera1n for tethered bootrom entry, 24kpwn for persistence on old bootrom 3GS / iPod touch 2G where comex’s kernel untether was not required.

## Not Pongo, not checkm8, not Recovery

| Lane | Era | purplepois0n |
|------|-----|--------------|
| **24kpwn** | 2009–2011, old BR 8720/8920 | `gen0-24kpwn` stub — delegate only |
| **limera1n** | 2010+, A4-class DFU | `gen0-limera1n` stub |
| **Recovery rdsk + `go`** | Signed restore chain | `recovery-boot-chain` |
| **Pongo + KPF + `bootx`** | checkra1n A5–A11 | `pongo-boot-chain` |

24kpwn is **DFU + crafted IMG3** — not iBoot `go`, not Pongo USB `4141`, not IM4P/TSS personalization.

## Chain shape (conceptual)

```mermaid
flowchart LR
    DFU[DFU bootrom]
    IMG3[Crafted LLB IMG3 0x24000 layout]
    LLB[Unsigned early boot]
    Untether[Persistent jailbreak after reboot]
    DFU --> IMG3 --> LLB --> Untether
```

Public write-ups describe:

1. Padding an IMG3 to the **0x24000** segment boundary.
2. Overwriting a **SHA1 hardware register** destination so hashed data lands on the stack.
3. Replacing the function **LR** so execution jumps into payload bytes embedded in the IMG3 **DATA** tag.

The final exploit IMG3 layout was verified under **planetbeing**’s design with **posixninja** (see Apple Wiki article).

## Supported hardware (public)

| Device | CPID (typical) | Notes |
|--------|----------------|-------|
| iPod touch 2G (**old bootrom**) | `0x8720` | S5L8720 |
| iPhone 3GS (**old bootrom**) | `0x8920` | Distinct from new-bootrom `0x8930` |
| iPhone 3GS (new bootrom) | `0x8930` | **Not** a 24kpwn untether target |

iOS bands in community tools: roughly **3.x** through early **4.x** on those models; pairing with **limera1n** extended use into the iOS 4 greenpois0n era on old-BR 3GS.

## purplepois0n status

| Component | Status |
|-----------|--------|
| `Kpwn24kExploitModule` (`gen0-24kpwn`) | **Stub** — probe/delegate via `ExploitModulePrimitive` |
| `DFUDevice` | Transport only — no IMG3 builder |
| Crafted 0x24000 IMG3 generator | **NOT in-tree** |
| `PURPLEPOIS0N_24KPWN` | Env path to external tool/binary (same delegate pattern as limera1n) |

Registration: [`PrimitiveRegistry.cpp`](../../../src/primitives/PrimitiveRegistry.cpp) alongside `gen0-limera1n`. On DFU `--gen0`, the module runs in the Probe stage when CPID is `8720`/`8920` (or CPID unknown).

**Honest boundary:** no exploit bytes, no redsn0w bundle, no automatic old-vs-new bootrom detection beyond CPID hints.

## vs limera1n

| | 24kpwn | limera1n |
|---|--------|----------|
| **Author lineage** | planetbeing / Chronic Dev | geohot |
| **Devices** | Old BR 8720/8920 | A4-class + broader DFU set |
| **Persistence** | **Untether** on supported old BR | Tethered bootrom; untether via separate userland exploit |
| **Mechanism** | IMG3 SHA1 segment overflow | Different USB control-transfer bug |
| **Host staging** | Crafted IMG3 in DFU | geohot checkra1n-era USB sequence |

greenpois0n RC5+ on **iPhone 4** used **limera1n + kernel untether**; old-BR **3GS** paths in the same era often referenced **24kpwn** in community wikis and redsn0w bundles.

## Related reading

- [dfu-recovery.md](dfu-recovery.md) — `DFUDevice` transport
- [primitives-gen0.md](primitives-gen0.md) — historical exploit module table
- [00-chronic-dev-greenpois0n.md](../00-chronic-dev-greenpois0n.md) — greenpois0n era context
- [recovery-ramdisk.md](recovery-ramdisk.md) — Pongo vs Recovery (unrelated lane)
- [Jailbreak Exploits — The Apple Wiki](https://theapplewiki.com/wiki/Jailbreak_Exploits)
