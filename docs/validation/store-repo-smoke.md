# purplepois0n-store — host repo smoke & HTTPS deploy

## Offline smoke (CI / dev)

```bash
make smoke-dpkg-store
```

Covers: `--store-init`, `--store-build`, `--store-publish`, subcommand `store build`, `dpkg-store` primitive.

Seed catalog with smoke + zebra packages:

```bash
legacy/scripts/seed-store.sh
```

## Host workflow (subcommands)

```bash
./build/bin/purplepois0n store init
legacy/scripts/seed-store.sh
./build/bin/purplepois0n store publish ./store-publish
```

Legacy flags still work with deprecation warnings.

## HTTPS deploy

```bash
export PURPLEPOIS0N_REPO_URL=https://YOUR_HOST/purplepois0n-repo/
legacy/scripts/seed-store.sh
legacy/scripts/deploy-https-repo.sh ./store-publish
```

Upload `store-publish/` to static HTTPS host (nginx, S3, Cloudflare R2, GitHub Pages).

## Device apt source (HTTPS)

After jailbreak with Procursus/Dopamine (`/var/jb` + `apt`):

```
deb [trusted=yes] https://YOUR_HOST/purplepois0n-repo/ purplepois0n main
```

Write to `/var/jb/etc/apt/sources.list.d/purplepois0n.list`, then:

```bash
apt update
apt install purplepois0n-smoke purplepois0n-zebra
```

## Device apt source (SSH file repo)

```bash
./build/bin/purplepois0n store sync -d UDID
```

Installs:

```
deb [trusted=yes] file:///var/jb/var/mobile/purplepois0n-store purplepois0n main
```

## Post-jailbreak pipeline

```bash
./build/bin/purplepois0n post-jb \
  --store \
  --store-install purplepois0n-zebra \
  --normal-ssh -d UDID
```

Requires `make plugins`. Probes `/var/jb`, syncs repo, optional `apt install`.

## Hardware validation

| Path | Command | Status |
|------|---------|--------|
| A HTTPS | Manual apt source + `apt install` | Run on Dopamine device |
| B SSH sync | `PURPLEPOIS0N_DEVICE_UDID=… tests/smoke_store_device.sh` | Optional CI skip |
| C post-jb | `post-jb --store --store-install purplepois0n-smoke -d UDID` | Manual |

**CI note:** `smoke_store_device.sh` skips when `PURPLEPOIS0N_DEVICE_UDID` is unset.

## Web UI

```bash
make agent    # terminal 1
make web-dev  # terminal 2
```

Jailbreak wizard step 5: **Sync & install store** pushes repo and installs `purplepois0n-zebra` when present in catalog.
