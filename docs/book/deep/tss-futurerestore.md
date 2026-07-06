# Deep dive: TSS signing, idevicerestore, and futurerestore

**Depth:** L5  
**Sources:** libimobiledevice `idevicerestore` (`tss.c`), tihmstar `futurerestore`, purplepois0n `TssDelegate`, `external/ipsw/pkg/tss`

Apple restore loads **personalized** IMG4/IMG3 components. The host must obtain signing tickets from the TSS controller (`gs.apple.com`) or from a **saved APTicket** (`.shsh` / `.shsh2`).

## Two pipelines

| Pipeline | Tool | When |
|----------|------|------|
| **Stock / live** | idevicerestore + libtss | Apple still signs the target build; device connected; manifests from target IPSW |
| **Saved ticket** | futurerestore | Target build **unsigned**; you hold a valid APTicket; SEP/baseband strategy required |

purplepois0n does **not** embed libtss yet. It probes and delegates:

- **ipsw** — `download tss --signed` (host signing check)
- **idevicerestore** — `PURPLEPOIS0N_IDEVICERESTORE`
- **futurerestore** — `PURPLEPOIS0N_FUTURERESTORE` (implements process changes below)

## idevicerestore (stock)

1. Parse BuildManifest from IPSW  
2. Read device identifiers (ECID, board/chip, nonce from iBoot when in recovery)  
3. Build TSS request plist → POST to Apple  
4. Receive ApImg4Ticket / per-component signatures  
5. Personalize iBSS, iBEC, kernelcache, etc.  
6. Upload via libirecovery; run restore FSM  

purplepois0n uses libirecovery **directly** for I/O; full FSM remains out of tree.

## futurerestore (process changes)

futurerestore is a **wrapper** around idevicerestore for **non-matching** firmware:

### Required beyond stock restore

| Change | Flags | Why |
|--------|-------|-----|
| Saved APTicket | `-t` / `--apticket` | Apple no longer signs target IPSW live |
| SEP ticket from *another* build | `--latest-sep` or `-s` + `-m` | SEP must match target iOS compatibility |
| Baseband ticket from *another* build | `--latest-baseband` or `-b` + `-p` | Same for cellular devices |
| No baseband | `--no-baseband` | iPod touch, Wi‑Fi iPads |

### Optional behavior

| Flag | Effect |
|------|--------|
| `--update` | OTA-style update install (needs matching ticket; not from jailbroken OS) |
| `-w` / `--wait` | ApNonce collision (Prometheus); reboot until nonce matches ticket |
| `--use-pwndfu` | Odysseus path with libipatcher |
| `--just-boot` | Tethered boot from pwned DFU only |
| `-e` / `--exit-recovery` | Exit recovery mode and quit without restoring |
| `-d` / `--debug` | Verbose logging (CLI: `--debug-restore`; purplepois0n `-d` is device UDID) |

### Methods (educational)

- **Prometheus** — generator in NVRAM or `-w` collision; ticket with generator/nonce  
- **Odysseus** — pwnDFU + OTA/saved blobs + `--use-pwndfu`  
- **alitek no-nonce** — iOS 9.x tickets without ApNonce (32-bit)  

See [futurerestore README](https://github.com/tihmstar/futurerestore/blob/master/README.md).

## purplepois0n integration

### Primitive

`TssSigningProbePrimitive` (`tss-signing-probe`) runs in ChainRunner **Probe** when IPSW, APTicket, or TSS tools are configured.

### Environment

| Variable | Purpose |
|----------|---------|
| `PURPLEPOIS0N_IDEVICERESTORE` | idevicerestore binary |
| `PURPLEPOIS0N_FUTURERESTORE` | futurerestore binary |
| `PURPLEPOIS0N_IPSW` | ipsw for `download tss` |
| `PURPLEPOIS0N_APTICKET` | Default APTicket path |
| `PURPLEPOIS0N_TSS_MODE` | `stock`, `futurerestore`, or `auto` |
| `PURPLEPOIS0N_FUTURERESTORE_LATEST_SEP` | Set `1` for `--latest-sep` |
| `PURPLEPOIS0N_FUTURERESTORE_LATEST_BASEBAND` | Set `1` for `--latest-baseband` |
| `PURPLEPOIS0N_FUTURERESTORE_NO_BASEBAND` | Set `1` for `--no-baseband` |
| `PURPLEPOIS0N_FUTURERESTORE_SEP` / `_SEP_MANIFEST` | Manual SEP + manifest |
| `PURPLEPOIS0N_FUTURERESTORE_BASEBAND` / `_BASEBAND_MANIFEST` | Manual BB + manifest |
| `PURPLEPOIS0N_SEP_IPSW` | IPSW for latest SEP BuildManifest (libtatsu / `--latest-sep`) |
| `PURPLEPOIS0N_BB_IPSW` | IPSW for latest baseband BuildManifest |
| `PURPLEPOIS0N_LATEST_IPSW` | Fallback when `--latest-sep` / `--latest-baseband` without explicit SEP/BB IPSW |
| `PURPLEPOIS0N_FUTURERESTORE_ARGS` | Extra futurerestore argv |
| `PURPLEPOIS0N_FUTURERESTORE_UPDATE` / `_WAIT_NONCE` / `_USE_PWNDFU` / `_JUST_BOOT` | Match `--update`, `-w`, `--use-pwndfu`, `--just-boot` |
| `PURPLEPOIS0N_FUTURERESTORE_EXIT_RECOVERY` / `_DEBUG` | Match `--exit-recovery`, `--debug-restore` |
| `PURPLEPOIS0N_IDEVICERESTORE_UPDATE` / `_DEBUG` | idevicerestore `-u` / `-d` |

### futurerestore parity matrix

| futurerestore flag | purplepois0n CLI | Env | Notes |
|--------------------|------------------|-----|-------|
| `-t` / `--apticket` | `--apticket` (repeatable) | `PURPLEPOIS0N_APTICKET` | Multiple tickets for ApNonce collision |
| `--update` | `--update` | `PURPLEPOIS0N_FUTURERESTORE_UPDATE` | Also sets idevicerestore `-u` |
| `-w` / `--wait` | `--wait` | `PURPLEPOIS0N_FUTURERESTORE_WAIT_NONCE` | |
| `--debug` | `--debug-restore` | `PURPLEPOIS0N_FUTURERESTORE_DEBUG` | Avoid `-d` (device UDID) |
| `-e` / `--exit-recovery` | `--exit-recovery` | `PURPLEPOIS0N_FUTURERESTORE_EXIT_RECOVERY` | |
| `--use-pwndfu` | `--use-pwndfu` | `PURPLEPOIS0N_FUTURERESTORE_USE_PWNDFU` | Odysseus |
| `--just-boot` | `--just-boot` + `--just-boot-args` | `_JUST_BOOT`, `_JUST_BOOT_ARGS` | |
| `--latest-sep` | `--latest-sep` | `PURPLEPOIS0N_FUTURERESTORE_LATEST_SEP` | |
| `-s` / `-m` | `--sep` / `--sep-manifest` | `_SEP` / `_SEP_MANIFEST` | |
| `--latest-baseband` | `--latest-baseband` | `PURPLEPOIS0N_FUTURERESTORE_LATEST_BASEBAND` | |
| `-b` / `-p` | `--baseband` / `--baseband-manifest` | `_BASEBAND` / `_BASEBAND_MANIFEST` | |
| `--no-baseband` | `--no-baseband` | `PURPLEPOIS0N_FUTURERESTORE_NO_BASEBAND` | |
| Restore spawn | `--futurerestore-restore --i-understand-restore` | `PURPLEPOIS0N_FUTURERESTORE` | `make plugins` |
| Stock live spawn | `--idevicerestore-restore --i-understand-restore` | `PURPLEPOIS0N_IDEVICERESTORE` | `make plugins` |

Offline argv smoke: `make smoke-futurerestore-parity`.

### CLI

```bash
# Signing status (Normal device, lockdown metadata)
./build/bin/purplepois0n --tss-check -d <UDID>

# Prometheus collision with multiple tickets
./build/bin/purplepois0n --gen0 --ipsw firmware.ipsw \
  --apticket t1.shsh2 --apticket t2.shsh2 --wait --latest-sep --latest-baseband

# Destructive restore (make plugins + explicit ack)
./build/bin/purplepois0n --futurerestore-restore --i-understand-restore \
  --ipsw firmware.ipsw --apticket blob.shsh2 --latest-sep --latest-baseband
./build/bin/purplepois0n --idevicerestore-restore --i-understand-restore --ipsw signed.ipsw
```

Full **futurerestore restore** spawn is mutation-gated (`make plugins`); default runs are probe-only.

## Phase 7.5 — Recovery upload (implemented)

| Piece | Role |
|-------|------|
| `RecoveryDevice::sendFile` | libirecovery upload in Recovery |
| `RecoveryUploadPrimitive` | personalize (ipsw) + upload when Recovery + paths set |
| `TssDelegate::extractIm4mFromApticket` | `ipsw img4 im4m extract` |
| `TssDelegate::personalizeComponent` | `ipsw img4 person` |

Pipeline:

1. **Stock live:** idevicerestore/libtss → IM4M → `ipsw img4 person` → `irecv_send_file`  
2. **futurerestore:** saved APTicket → extract IM4M → personalize unsigned iBSS from IPSW → upload  
3. **Pre-signed:** `--recovery-upload signed.img4` → upload only (probe logs command; execute needs `make plugins` + mutation)

```bash
# Recovery device: probe upload plan
./build/bin/purplepois0n --gen0 --recovery-upload /path/iBSS.signed --recovery-component iBSS

# Personalize from ticket + unsigned component, then upload (execute gated)
./build/bin/purplepois0n --gen0 --apticket blob.shsh2 --ipsw-component iBSS.im4p \
  --recovery-component iBSS --report /tmp/pp.json
```

| `PURPLEPOIS0N_RECOVERY_UPLOAD`, `PURPLEPOIS0N_IM4M_MANIFEST`.

Env (execute path): `PURPLEPOIS0N_RECOVERY_RESET=1`, `PURPLEPOIS0N_RECOVERY_REBOOT=1` after upload.

Device validation checklist: [validation/tss-recovery-smoke.md](../../validation/tss-recovery-smoke.md).

**Still open:** in-tree idevicerestore FSM (DFU→ASR); Prometheus NVRAM generator automation; encrypted backup decrypt.

### libtatsu (in-tree live TSS)

Apple renamed/split **libtss** → **[libtatsu](https://github.com/libimobiledevice/libtatsu)** (your idevicerestore `tss.c` lineage). purplepois0n links it when available:

```bash
brew install libtatsu   # or build from source
make release              # auto-detects libtatsu-1.0 via pkg-config
make release LIBTATSU=1   # force enable

# Vendored prefix (no brew):
make external-libtatsu
PKG_CONFIG_PATH=$PWD/external/libtatsu-install/lib/pkgconfig make release LIBTATSU=1
```

`TssClient::fetchLiveShsh()`:
1. **libtatsu** — extract `BuildManifest.plist` from `--ipsw`, match BuildIdentity, add AP + SEP + baseband tags (`tss_request_add_se_tags` / `tss_request_add_baseband_tags`), `tss_request_send`
2. **Fallback** — `ipsw download tss --usb/--device`

For **futurerestore-style** SEP/baseband tickets, pass companion IPSWs:

| CLI | Env | libtatsu behavior |
|-----|-----|-------------------|
| `--latest-sep --sep-ipsw PATH` | `PURPLEPOIS0N_SEP_IPSW` | SEP tags from latest signed IPSW manifest |
| `--latest-baseband --bb-ipsw PATH` | `PURPLEPOIS0N_BB_IPSW` | Baseband tags from companion IPSW |
| `--no-baseband` | `PURPLEPOIS0N_FUTURERESTORE_NO_BASEBAND` | Skip baseband tags |

Stock live (no futurerestore flags): SEP and baseband tags come from the **target** IPSW when present.

IM4M extract uses **libplist** first, then `ipsw img4 im4m extract`.

```bash
# Recovery device + target IPSW (live nonce from irecv)
./build/bin/purplepois0n --fetch-shsh /tmp/live.shsh --ipsw firmware.ipsw

# Full Recovery upload chain (personalize + probe upload)
./build/bin/purplepois0n --gen0 --ipsw firmware.ipsw --ipsw-component iBSS.im4p \
  --recovery-component iBSS --report /tmp/pp.json
```

## Related

- [primitives-gen0.md](primitives-gen0.md) — ChainRunner  
- [dfu-recovery.md](dfu-recovery.md) — irecv upload  
- [BACKPORT_MATRIX.md](../../BACKPORT_MATRIX.md) — TSS row  
