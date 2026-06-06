# TSS / Recovery device validation

**Last run:** 2026-06-04 (automated build + offline fixtures; hardware pending)

Structured checklist from Phase 7 balanced sprint. Record pass/fail when running against a trusted device.

## Build prerequisites

```bash
# libtatsu (pick one)
make external-libtatsu
PKG_CONFIG_PATH=$PWD/external/libtatsu-install/lib/pkgconfig make release LIBTATSU=1

# or
brew install libtatsu && make release LIBTATSU=1

# ipsw fallback (optional)
make external-ipsw
```

| Check | Status | Notes |
|-------|--------|-------|
| `make release` (no libtatsu) | **Pass** | ipsw fallback path |
| `make release LIBTATSU=1` | **Pass** | Vendored prefix via `make external-libtatsu`; `--tss-check` logs `libtatsu: linked` |
| `make test-fixtures` | **Pass** | Includes `--analyze-crash` fixture |

## Hardware checklist

| # | Scenario | Command | Expected | Status |
|---|----------|---------|----------|--------|
| 1 | TSS signing probe (Normal) | `./build/bin/purplepois0n --tss-check -d UDID` | `ipsw download tss --signed` logged | **Not run** — no device in session |
| 2 | Live SHSH (Recovery) | `./build/bin/purplepois0n --fetch-shsh /tmp/live.shsh --ipsw TARGET.ipsw` | libtatsu + ApNonce in logs | **Not run** |
| 3 | ipsw fallback | Rebuild with `LIBTATSU=0`; same as #2 | SHSH via `ipsw download tss` | **Not run** |
| 4 | Recovery personalize probe | `./build/bin/purplepois0n --gen0 --ipsw ... --ipsw-component iBSS.im4p --recovery-component iBSS --report /tmp/pp.json` | Personalize plan + ApNonce; no upload without plugins | **Not run** |
| 5 | futurerestore SEP plan | `--gen0 --apticket blob.shsh2 --latest-sep --sep-ipsw LATEST.ipsw ...` | `logFuturerestoreSepBbPlan` shows companion IPSW | **Not run** |
| 6 | Recovery upload execute | `make plugins` + `--gen0 --recovery-upload signed.img4 --recovery-component iBSS` + `PURPLEPOIS0N_RECOVERY_REBOOT=1` | `irecv_send_file` + optional reboot | **Not run** |
| 7 | mobilebackup2 probe | `./build/bin/purplepois0n --gen0 -d UDID` (Normal) | `[MB2] negotiated protocol version` in chain | **Not run** |

## Known gaps (non-blockers until hardware fails)

- **SavedApTicket mode:** SEP/BB tickets delegated to futurerestore spawn; libtatsu live fetch is AP-focused in stock-live path.
- **Wi‑Fi-only devices:** baseband tag failures should log and continue (verify on iPod/Wi‑Fi iPad).
- **7.5 follow-on:** multi-stage iBSS→iBEC→kernel boot chain not automated; caller drives `PURPLEPOIS0N_RECOVERY_RESET` / `_REBOOT` after upload.
- **7.9:** CPID human-readable names in `-l` output still planned.

## Environment (Recovery upload)

| Variable | Effect |
|----------|--------|
| `PURPLEPOIS0N_RECOVERY_RESET` | `irecv_reset` after each chain upload (**default on**; set `=0` to opt out) |
| `PURPLEPOIS0N_RECOVERY_REBOOT=1` | `irecv_reboot` after upload (`RecoveryUploadPrimitive`) |

## Host automation

```bash
make smoke-tss    # TSS probe + futurerestore guard + pongo report JSON
make test-fixtures # includes assert_chain_report.sh for pongo-boot probe
```

## Related

- [book/deep/tss-futurerestore.md](../book/deep/tss-futurerestore.md)
- [book/deep/dfu-recovery.md](../book/deep/dfu-recovery.md)
- [legacy/INTEGRATION_PLAN.md](../legacy/INTEGRATION_PLAN.md) Phase 7
