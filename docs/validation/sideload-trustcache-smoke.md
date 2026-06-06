# Sideload + trustcache smoke checklist

Host and device validation for the sign → install → trustcache workflow.

## Build prerequisites

```bash
make release
make external-ipsw   # for --sign-ipa
make plugins         # for execute paths
```

## Host automation (CI)

```bash
make test-fixtures
make smoke-tss
```

## Hardware checklist

| # | Scenario | Command | Expected | Status |
|---|----------|---------|----------|--------|
| 1 | Sign IPA (host) | `./build/bin/purplepois0n --sign-ipa app.ipa --output /tmp/signed.ipa --ad-hoc` | Signed IPA on disk | **Not run** |
| 2 | Install probe | `./build/bin/purplepois0n --install-ipa /tmp/signed.ipa -d UDID` (no plugins) | instproxy handshake; no install | **Not run** |
| 3 | Install execute | `make plugins` + same as #2 | App appears on device | **Not run** |
| 4 | Trustcache (host jbctl) | `./build/bin/purplepois0n --trustcache-add /path/to/binary` | jbctl spawn or probe log | **Not run** |
| 5 | Trustcache (ramdisk SSH) | `--ramdisk-transport ssh -d UDID --trustcache-add BIN` | Upload + `jbctl trustcache add` over SSH | **Not run** |
| 6 | Full pipeline | `./build/bin/purplepois0n --post-jb-pipeline --sign-ipa in.ipa --install-ipa in.ipa --trustcache-add Payload.app/binary -d UDID` | sign → install → trustcache | **Not run** |
| 7 | Gen0 pipeline | `./build/bin/purplepois0n --gen0 --post-jb-pipeline ... -d UDID` | Probe chain then pipeline execute | **Not run** |

## CLI flags

| Flag | Purpose |
|------|---------|
| `--post-jb-pipeline` | Orchestrate sign-ipa (optional) → install-ipa → trustcache-add |
| `--sign-ipa` / `--install-ipa` / `--trustcache-add` | Individual stages (also composable) |
| `--ramdisk-transport ssh` | On-device trustcache via `RamdiskClient` instead of host jbctl |

## Related

- [book/deep/sideload-codesign.md](../book/deep/sideload-codesign.md)
- [SUPPORT.md](../SUPPORT.md) — sideload / trustcache rows
