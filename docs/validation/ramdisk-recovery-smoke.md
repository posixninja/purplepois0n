# Ramdisk + Recovery hardware smoke checklist

Manual validation on a lab device with saved SHSH/IM4M. No automated CI coverage yet.

## Prerequisites

- `make release` and `make external-ipsw`
- `make plugins` for mutating Recovery uploads (`PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS`)
- Device in **Recovery** with known ECID
- IPSW matching device + apticket (`--apticket` or live TSS)

## 1. In-memory HFS+ builder (offline)

```bash
./tests/run_fixtures.sh
```

Expect `ipsw disk hfs` to list overlay files (`hello.txt`).

## 2. Stock + overlay merge (offline)

```bash
./build/bin/purplepois0n --ramdisk-from-ipsw --ipsw firmware.ipsw \
  --ramdisk-overlay ./my-overlay --ramdisk-work-dir /tmp/pp-rdsk \
  --output /tmp/custom.im4p
./external/ipsw/ipsw disk hfs /tmp/pp-rdsk/modified.dmg | head
```

Confirm stock restore paths remain **and** overlay files appear.

## 3. Recovery chain probe (device, no upload)

```bash
./build/bin/purplepois0n --gen0 --recovery-chain --ipsw firmware.ipsw \
  -d UDID --report /tmp/recovery-probe.json
```

Expect `recovery-boot-chain` success in report; uploads logged as "would upload".

### Automated report assertion (CI, no device)

```bash
./tests/assert_chain_report.sh /tmp/pongo-probe.json \
  --pongo-boot --pongo-kpf /tmp/x --pongo-ramdisk /tmp/x.dmg
```

Parses `--report` JSON and verifies non-empty `reports[]` with `stage`, `result`, `message`.

Inter-stage `irecv_reset` defaults **on** after each upload (`PURPLEPOIS0N_RECOVERY_RESET=0` to opt out). Failed `go` returns non-zero exit when `--recovery-execute` + `make plugins`.

## 4. Recovery chain execute (device, mutation)

```bash
./build/bin/purplepois0n --gen0 --recovery-execute --ipsw firmware.ipsw \
  --apticket blob.shsh2 --ramdisk-overlay ./overlay -m --report /tmp/recovery-exec.json
```

Expect irecv uploads for iBSS/iBEC/rdsk; optional `go` unless `PURPLEPOIS0N_RECOVERY_BOOT=0`.

## 5. Post-boot TCP agent (custom rdsk)

After custom ramdisk boots and usbmux enumerates:

```bash
iproxy -u UDID 4444:4444
./build/bin/purplepois0n -d UDID --ramdisk-probe
./build/bin/purplepois0n -d UDID --ramdisk-exec "uname -a"
```

See [tests/fixtures/pp-agent/README.md](../../tests/fixtures/pp-agent/README.md).

## Sign-off

When all steps pass on hardware, update [BACKPORT_MATRIX.md](../BACKPORT_MATRIX.md) §7.12 from **Partial** → **Done** for in-tree builder + chain; live comm remains user-agent dependent.
