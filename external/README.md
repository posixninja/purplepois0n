# External dependencies (git submodules)

| Path | Upstream | Purpose |
|------|----------|---------|
| `ipsw/` | [blacktop/ipsw](https://github.com/blacktop/ipsw) | Mach-O / dyld cache / IPSW analysis (`ipswd` REST + `ipsw` CLI) |

## Clone and build

```bash
git submodule update --init --recursive external/ipsw
make -C external/ipsw build
# binary: external/ipsw/ipsw
```

## ipswd (preferred)

purplepois0n talks to **ipswd** over HTTP (default `http://127.0.0.1:3993/v1`) for Mach-O and dyld shared cache info. Requires `curl` on the host and a running daemon.

```bash
make external-ipswd
./external/ipsw/ipswd start
curl -sS http://127.0.0.1:3993/v1/_ping
```

Set `PURPLEPOIS0N_IPSWD` to override the base URL (with or without `/v1` suffix).

## ipsw CLI (fallback)

When ipswd is not reachable, purplepois0n resolves the CLI in this order:

1. `PURPLEPOIS0N_IPSW` environment variable (explicit path)
2. `external/ipsw/ipsw` (submodule build)
3. `ipsw` on `PATH` (e.g. Homebrew)

Requires **Go**, **network** (for `go mod download` via proxy.golang.org on first build), and **CGO** (libusb, etc.) — see upstream README.

If `make external-ipsw` fails with `lookup proxy.golang.org: no such host`, retry on a machine with internet, or after `go mod download` in `external/ipsw`:

```bash
cd external/ipsw && CGO_ENABLED=1 go build -o ipsw ./cmd/ipsw
```
