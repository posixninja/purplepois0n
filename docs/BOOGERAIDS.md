# boogeraids handoff (purplepois0n ↔ ipsw / ipswd)

purplepois0n and [boogeraids](https://github.com/blacktop/ipsw) (via the sibling project `boogeraids/`) share the same **ipsw** analysis stack. purplepois0n prefers the **ipswd** REST daemon when it is running; boogeraids can use the same JSON schemas via `IPSWAgent` or ingested `--analyze-json` files.

## Analysis backends (Mach-O / dyld cache)

| Priority | Backend | How |
|----------|---------|-----|
| 1 | **ipswd** | `GET http://127.0.0.1:3993/v1/macho/info` or `/v1/dsc/info` (via `IpswdClient` + `curl`) |
| 2 | **ipsw** CLI | `ipsw macho info --json` / `ipsw dyld info --json` |
| 3 | **internal** | `MachOParser` / `DyldCacheParser` |

Start the daemon from the submodule:

```bash
make external-ipswd
./external/ipsw/ipswd start
curl -sS http://127.0.0.1:3993/v1/_ping   # OK
```

Override the API base URL:

```bash
export PURPLEPOIS0N_IPSWD=http://127.0.0.1:3993/v1
# or host only: export PURPLEPOIS0N_IPSWD=http://myhost:3993
```

## Shared ipsw CLI (fallback)

Resolution order for the CLI when ipswd is down:

1. `PURPLEPOIS0N_IPSW` environment variable (explicit path)
2. `purplepois0n/external/ipsw/ipsw` — submodule build (`make external-ipsw`)
3. `ipsw` on `PATH`

```bash
export PURPLEPOIS0N_IPSW=/path/to/purplepois0n/external/ipsw/ipsw
export PATH="/path/to/purplepois0n/external/ipsw:$PATH"
```

```bash
git submodule update --init external/ipsw
make external-ipsw
```

## `--analyze-json` export

```bash
./build/bin/purplepois0n --analyze-binary /bin/ls --analyze-json /tmp/ls-macho.json
./build/bin/purplepois0n --analyze-dyldcache /path/to/dyld_shared_cache_arm64 --analyze-json /tmp/dsc.json
```

CLI prints `Backend: ipswd` when the daemon answered; otherwise `ipsw` or `internal`.

- **ipswd / ipsw**: JSON file is the raw REST/CLI payload (same schema boogeraids `IPSWAgent` uses with `macho_info` + `json: true`).
- **internal**: minimal `purplepois0n-internal` summary — boogeraids should use `BinaryParserAgent` or call `IPSWAgent` / ipswd directly.

## Dyld cache

`GET /v1/dsc/info` (ipswd) or `ipsw dyld info --json` supplies the catalog JSON. Layout fields (mappings, image list) and `extractImage()` still use the internal `DyldCacheParser` when the handle loaded via ipswd/ipsw.

## boogeraids integration options

| Approach | When |
|----------|------|
| **Ingest `--analyze-json`** | purplepois0n already ran; store JSON under `analysis_output/<program>/` |
| **`IPSWAgent` / ipswd** | Live analysis; point agents at the same daemon URL or `ipsw` binary |
| **`BinaryParserAgent`** | No ipsw/ipswd; otool/lipo `parse_*.json` schema |

## References

- [external/README.md](../external/README.md) — submodule, `make external-ipsw`, `make external-ipswd`
- [docs/book/deep/binary-parsers.md](book/deep/binary-parsers.md) — opaque handles
- Upstream API: `external/ipsw/api/swagger.json` (host `localhost:3993`, base `/v1`)
