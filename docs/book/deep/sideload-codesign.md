# Host codesign and sideload (research lane)

purplepois0n does **not** ship CoreTrust exploit bytes or Fugu15 persistence. The supported research path is:

1. **Host** — sign Mach-O / `.app` / IPA with `ipsw macho sign` (bundled `external/ipsw`) and optional `ldid`
2. **USB** — install a signed IPA on a trusted Normal-mode device via `installation_proxy`
3. **On-device** — after a kernel jailbreak, add binaries to the trust cache via an external `jbctl` or `PURPLEPOIS0N_TRUSTCACHE_TOOL` delegate

## Fugu15 / Dopamine context

Fugu15 relied on **kernel exploits plus CoreTrust-class bugs** for persistence and ad-hoc execution on stock iOS. Dopamine documents post-kernel **`jbctl trustcache`** and BaseBin codesign bypass — see [puaf-kfd-era.md](puaf-kfd-era.md) and [Fugu15 on The Apple Wiki](https://theapplewiki.com/wiki/Fugu15).

purplepois0n maps the **host + delegate** slice only; reproducing CVE-2022-26766 or in-tree trust-cache mutation is out of scope.

## CLI

| Flag | Role |
|------|------|
| `--sign-macho PATH` | Ad-hoc or cert sign a single Mach-O |
| `--sign-app PATH` | Sign a `.app` bundle (directory input to ipsw) |
| `--sign-ipa PATH` | Unzip → sign `Payload/*.app` → repack |
| `--sign-id`, `--ent`, `--ad-hoc`, `--output` | Signing options |
| `--install-ipa PATH` | instproxy install (`-d UDID`; execute needs `make plugins`) |
| `--trustcache-add PATH` | Probe/run jbctl trustcache add |
| `--gen0` + above | Runs codesign / sideload / trust-cache **probes** in the chain |

## Environment

| Variable | Purpose |
|----------|---------|
| `PURPLEPOIS0N_CODESIGN_ENT` | Default entitlements plist |
| `PURPLEPOIS0N_CODESIGN_ID` | Default bundle identifier |
| `PURPLEPOIS0N_LDID` | Optional ldid path |
| `PURPLEPOIS0N_CODESIGN_ARGS` | Extra flags for `ipsw macho sign` |
| `PURPLEPOIS0N_INSTALL_IPA` | Default IPA for Gen0 sideload probe |
| `PURPLEPOIS0N_IDEVICEINSTALLER` | Use `ideviceinstaller` instead of in-process instproxy |
| `PURPLEPOIS0N_JBCTL` | Path to Dopamine `jbctl` on host |
| `PURPLEPOIS0N_TRUSTCACHE_TOOL` | Generic trust-cache CLI |
| `PURPLEPOIS0N_TRUSTCACHE_PATH` | Mach-O to add after sideload |

Example entitlements fixture (research only): `tests/fixtures/ents/jailbreak-helper.plist`.

## Honest boundaries

- **Stock iOS** rejects ad-hoc IPAs from free developer accounts and unsigned payloads; pairing and cert limits apply.
- **Trust cache execute** requires a completed jailbreak and `make plugins` for mutation paths.
- **TrollStore** / permanent marketplace install is not integrated — user installs Dopamine IPA via documented external flows.

## Primitives

| Primitive | Stage |
|-----------|--------|
| `codesign-signing-probe` | Probe — tool discovery + optional sign plan |
| `offline-codesign-patch` | Probe — offline patch surface; signs when `codesignInputPath` set |
| `sideload-install` | Injection — Normal mode instproxy |
| `gen6-trustcache` | TrustCache — delegate when `PURPLEPOIS0N_JBCTL` configured |

See [primitives-gen0.md](primitives-gen0.md) for chain registration.
