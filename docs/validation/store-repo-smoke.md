# purplepois0n-store — host repo smoke & HTTPS deploy

## Offline smoke (CI / dev)

```bash
make smoke-dpkg-store
```

Covers: `--store-init`, `--store-build`, `--store-publish`, `dpkg-store` primitive.

## Host workflow

```bash
./build/bin/purplepois0n --store-init
./build/bin/purplepois0n --store-add /path/to/package.deb
./build/bin/purplepois0n --store-build
./build/bin/purplepois0n --store-publish ./store-publish
```

Upload `store-publish/` to any static HTTPS host (nginx, S3, Cloudflare R2, GitHub Pages).

## Device apt source (HTTPS)

After jailbreak with Procursus/Dopamine (`/var/jb` + `apt`):

```
deb [trusted=yes] https://YOUR_HOST/ purplepois0n main
```

Write to `/var/jb/etc/apt/sources.list.d/purplepois0n.list`, then:

```bash
apt update
apt install your-package
```

## Device apt source (SSH file repo)

```bash
./build/bin/purplepois0n --store-sync --normal-ssh -d UDID
```

Installs:

```
deb [trusted=yes] file:///var/jb/var/mobile/purplepois0n-store purplepois0n main
```

## Post-jailbreak pipeline

```bash
./build/bin/purplepois0n --post-jb-pipeline \
  --install-ipa /path/to/app.ipa \
  --post-jb-store \
  --post-jb-store-install your-package \
  --normal-ssh -d UDID
```

Requires `make plugins`. Probes `/var/jb`, syncs repo, optional `apt install`.

## Manual validation on Dopamine device

1. Jailbreak with Dopamine; confirm `ssh mobile@device` via iproxy.
2. Publish repo to HTTPS; add source on device.
3. `apt update && apt install purplepois0n-smoke` (or your package).
4. **Status:** run when hardware available — not executed in CI.
