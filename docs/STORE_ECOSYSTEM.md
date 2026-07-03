# purplepois0n package ecosystem

Host-side tooling for a rootless apt repo under `/var/jb`, integrated with Procursus bootstrap and an on-device store fork (Zebra).

## Components

| Layer | Tool | Location |
|-------|------|----------|
| Host repo builder | `DpkgStore` | `src/store/` |
| CLI | `--store-*`, `--post-jb-store` | `src/purplepois0n.cpp` |
| Bootstrap probe | `RootlessBootstrapPrimitive` | `src/primitives/` |
| On-device store | Zebra fork (sibling) | `legacy/zebra-purple/` via setup script |
| Base system | Procursus mirror | `legacy/modern-era/Procursus` |

## Environment variables

| Variable | Purpose |
|----------|---------|
| `PURPLEPOIS0N_STORE_ROOT` | Host repo path (default `./store`) |
| `PURPLEPOIS0N_STORE_INSTALL` | Package to install when `dpkg-store` primitive runs |
| `PURPLEPOIS0N_JBROOT` | Device jbroot prefix (default `/var/jb`) |
| `PURPLEPOIS0N_JB_HELPER` | External jailbreak installer CLI |
| `PURPLEPOIS0N_PALERA1N_HELPER` | palera1n helper (rootless bootstrap) |
| `PURPLEPOIS0N_REPO_URL` | Default HTTPS repo URL baked into Zebra fork |
| `PURPLEPOIS0N_NORMAL_SSH` | Enable SSH sync path (`1`) |

## Procursus integration

1. Mirror upstream:

   ```bash
   legacy/scripts/procursus-mirror.sh
   ```

2. After jailbreak, purplepois0n delegates bootstrap to `PURPLEPOIS0N_JB_HELPER` / palera1n — Procursus strapping is **not** built in-tree.

3. Add purplepois0n repo as default source in your bootstrap overlay or post-jb pipeline (`--post-jb-store`).

## Zebra fork

```bash
legacy/scripts/zebra-fork-setup.sh
```

Creates `legacy/zebra-purple/` from [zbra-dev/Zebra](https://github.com/zbra-dev/Zebra). Set `PURPLEPOIS0N_REPO_URL` before building to point default sources at your HTTPS host.

Ship the built `.deb` via `--store-add` + `--store-publish`.

## Custom bootstrap (optional)

For a private Procursus package subset and per-SDK rebuilds, see `legacy/scripts/procursus-bootstrap-ci.sh` (skeleton CI).

## Related docs

- [validation/store-repo-smoke.md](validation/store-repo-smoke.md)
- [GENERATIONS.md](GENERATIONS.md) — scope boundaries
- [book/07-dopamine-rootless.md](book/07-dopamine-rootless.md)
