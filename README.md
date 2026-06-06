# purplepois0n

iOS Jailbreak Tool - A framework for interacting with iOS devices in various states and performing jailbreak operations.

## Overview

purplepois0n is a C++ jailbreak tool that provides a comprehensive framework for:
- Detecting and connecting to iOS devices in Normal, Recovery, and DFU modes
- Communicating with devices through various protocols
- Parsing dyld shared caches, Mach-O binaries, and iOS backups
- Performing jailbreak operations (exploit implementation required)

## Lineage

purplepois0n supersedes **greenpois0n** (Chronic Dev Team, DFU/userland tooling for iOS 4.x) and **absinthe** (iOS 5.0.1 / 5.1.1 untether era). Like those tools, it is a host-side utility that talks to devices over USB—but this repository is a **research framework** (device I/O, parsers, exploit hooks), not a shipping jailbreak for a specific iOS build. See [docs/LINEAGE.md](docs/LINEAGE.md) for history and [docs/GENERATIONS.md](docs/GENERATIONS.md) for later eras (evasi0n through Dopamine).

## Documentation

- [docs/README.md](docs/README.md) — Documentation index and reading order
- [docs/book/README.md](docs/book/README.md) — Educational jailbreak era book (chapters + sources)
- [docs/SUPPORT.md](docs/SUPPORT.md) — Gen 0 capability matrix (honest gaps vs greenpois0n/absinthe)
- [docs/LINEAGE.md](docs/LINEAGE.md) — greenpois0n, absinthe, and purplepois0n’s role
- [docs/GENERATIONS.md](docs/GENERATIONS.md) — Jailbreak generations, mitigations, and framework mapping

## Features

- **Multi-mode Device Support**: Works with devices in Normal, Recovery, and DFU modes
- **checkm8 (DFU)**: Bootrom exploit orchestration for A5–A11 via external [gaster](https://github.com/0x7ff/gaster) or [ipwndfu](https://github.com/axi0mX/ipwndfu)
- **Device Enumeration**: List and identify all connected iOS devices
- **File Transfer**: AFC (Apple File Conduit) service for file operations
- **Memory Operations**: Read/write memory in Recovery and DFU modes
- **Binary Parsing**: Parse dyld shared caches, Mach-O binaries, and iOS backups
- **Comprehensive Logging**: Built-in logging system with multiple severity levels
- **Command-line Interface**: Easy-to-use CLI with multiple options

## Architecture

```
purplepois0n (main)
    ↓
DeviceManager (detects device state)
    ↓
    ├─→ MobileDevice (Normal mode)
    ├─→ RecoveryDevice (Recovery mode)
    └─→ DFUDevice (DFU mode)
            ↓
        AFCService (file operations when in normal mode)
```

### Core Components

- **DeviceManager**: Central device detection and enumeration
- **MobileDevice**: Interface for devices in normal iOS mode
- **RecoveryDevice**: Interface for devices in recovery mode
- **DFUDevice**: Interface for devices in DFU (Device Firmware Update) mode
- **AFCService**: File transfer service for normal mode devices
- **DyldCacheParser**: Parser for dyld shared cache files
- **MachOParser**: Parser for Mach-O binary files
- **MobileBackup**: Parser for iOS backup files (Manifest.plist, Manifest.mbdb, Manifest.db)
- **Logger**: Logging utility with configurable levels

## Dependencies

### Required Libraries

- **libimobiledevice**: iOS device communication library
- **libirecovery**: Recovery and DFU mode communication (`brew install libirecovery libimobiledevice` on macOS). Headers are `<libirecovery.h>` and `<libimobiledevice/...>` (not legacy `irecovery.h` / `instproxy.h` names).
- **libplist**: Property list handling
- **sqlite3**: Manifest.db backup index (system library on macOS)
- **libusbmuxd**: USB multiplexing daemon

On Apple Silicon macOS, the Makefile auto-adds Homebrew include/lib paths when `brew --prefix` is available.

Host builds default to the native CPU (`uname -m`). For a specific slice or universal binary:

```bash
make ARCH=arm64 release          # Apple Silicon
make ARCH=x86_64 release         # Intel macOS (needs x86_64 Homebrew deps)
make ARCHS="arm64 x86_64" release  # universal (requires fat/universal libs)
```

DFU/Recovery memory I/O uses standard 32-bit USB address encoding (`wValue`/`wIndex`) for both 32-bit (A4–A6) and 64-bit (A7+) bootrom targets.

### Build Tools

- C++14 compatible compiler (g++ or clang++)
- make
- autotools (for building dependencies)
- pkg-config

## Building

### Step 1: Install Dependencies

You can use either `bootstrap.sh` or `build.sh` to download and compile dependencies:

```bash
# Option 1: Use bootstrap.sh
./bootstrap.sh

# Option 2: Use build.sh
./build.sh
```

Both scripts will:
1. Clone the required repositories
2. Build and install the libraries
3. Install them system-wide (requires sudo)

### Step 2: Build purplepois0n

```bash
# Build release version (default)
make

# Or build debug version
make debug

# Build with in-tree mutating primitive gate enabled (still probe-only unless -m)
make plugins

# Clean build artifacts
make clean
```

The executable will be created at `build/bin/purplepois0n`.

### Step 3: Install (Optional)

```bash
sudo make install
```

This installs the binary to `/usr/local/bin/purplepois0n`.

## Usage

### Basic Usage

```bash
# Detect device and run probe scaffold (DFU: probe only; use -m for checkm8)
./build/bin/purplepois0n

# Run checkm8 only (device must be in DFU; requires gaster or ipwndfu on PATH)
./build/bin/purplepois0n --checkm8

# List all connected devices
./build/bin/purplepois0n --list

# Target specific device by UDID
./build/bin/purplepois0n --device <UDID>

# Enable verbose logging
./build/bin/purplepois0n --verbose
```

### Command-line Options

- `-h, --help`: Show help message
- `-v, --verbose`: Enable verbose/debug logging
- `-l, --list`: List all connected devices
- `-d, --device UDID`: Target specific device by UDID (normal mode)
- `-j, --jailbreak`: Default action — DFU runs primitive probe chain; other modes use Gen 0 scaffold
- `-m, --checkm8`: Run checkm8 bootrom exploit (DFU; needs gaster or ipwndfu on PATH)
- `--gen0`: Gen 0 scaffold with explicit Generation 0 messaging (no checkm8)
- `--analyze-backup PATH`: Offline backup manifest — v1 (mbdb/plist) or v2 (Manifest.db); flat/sharded paths
- `--afc-list REMOTE`: List AFC directory (requires `-d UDID`, Normal mode)
- `--afc-push LOCAL REMOTE`: Upload file via AFC (requires `-d UDID`, Normal mode)
- `--afc-pull REMOTE LOCAL`: Download file via AFC (requires `-d UDID`, Normal mode)
- `--tss-check`: Probe TSS signing tools; check if build is still signed (use with `-d UDID` in Normal mode)
- `--ipsw PATH`: Target IPSW for TSS / futurerestore probes in `--gen0`
- `--apticket PATH`: Saved SHSH/APTicket for futurerestore-style restore planning
- `--latest-sep`, `--latest-baseband`, `--no-baseband`: futurerestore SEP/baseband options (see [docs/book/deep/tss-futurerestore.md](docs/book/deep/tss-futurerestore.md))
- `--analyze-binary PATH`: Offline Mach-O summary; use `--arch arm32|arm64` for fat binaries
- `--analyze-dyldcache PATH`: Offline dyld shared cache summary (image catalog, mappings)
- `--analyze-json FILE`: Export JSON payload from analyze-binary/dyldcache (ipsw when available)

Environment variables:

| Variable | Purpose | Suggested pin |
|----------|---------|----------------|
| `PURPLEPOIS0N_GASTER` | Path to [gaster](https://github.com/0x7ff/gaster) binary (DFU checkm8) | gaster built from current `master`; smoke-tested with purplepois0n `-m` |
| `PURPLEPOIS0N_IPWNDFU` | Directory containing ipwndfu checkout (alternative to gaster) | [axi0mX/ipwndfu](https://github.com/axi0mX/ipwndfu) — study mirror: `legacy/OpenJailbreak/ipwndfu` |
| `PURPLEPOIS0N_IPSW` | Override ipsw binary for `--analyze-binary` / `--analyze-dyldcache` | Prefer submodule build below |
| `PURPLEPOIS0N_DOPAMINE_EXPLOITS` | Directory of built Dopamine exploit `.framework` bundles (e.g. `Dopamine.app/Frameworks`) | Build Dopamine separately; host dlopen requires matching architecture |
| `PURPLEPOIS0N_DOPAMINE_FLAVOR` | Default `exploit_init` flavor for all modules | kfd default: `physpuppet` (also `smith`, `landa`) |
| `PURPLEPOIS0N_DOPAMINE_FLAVOR_KFD` | Per-module flavor override | e.g. `smith` |
| `PURPLEPOIS0N_DOPAMINE_KFD` | Direct path to one exploit dylib (bypasses framework search) | e.g. `…/kfd.framework/kfd` |
| `PURPLEPOIS0N_JB_HELPER` | Path to external jailbreak installer/bootstrap CLI | Spawned from bootstrap stage when mutation enabled |
| `PURPLEPOIS0N_JB_HELPER_ARGS` | Extra args for JB helper (space-separated) | Optional |
| `PURPLEPOIS0N_LIMERA1N` | Path to limera1n exploit dylib (Gen 0 DFU) | Historical delegate |
| `PURPLEPOIS0N_EVASI0N` | Path to evasi0n exploit dylib (Gen 1 Normal) | Historical delegate |
| `PURPLEPOIS0N_CHECKRA1N` | Path to checkra1n helper (Gen 5 DFU) | Historical delegate |
| `PURPLEPOIS0N_IDEVICERESTORE` | idevicerestore binary (live TSS / signed restore) | libimobiledevice build |
| `PURPLEPOIS0N_FUTURERESTORE` | futurerestore binary (saved APTicket + SEP/BB) | [tihmstar/futurerestore](https://github.com/tihmstar/futurerestore) |
| `PURPLEPOIS0N_APTICKET` | Default `.shsh` / `.shsh2` path | Used with futurerestore probes |
| `PURPLEPOIS0N_TSS_MODE` | `stock`, `futurerestore`, or `auto` | Default `auto` |
| `PURPLEPOIS0N_FUTURERESTORE_*` | SEP/BB/latest flags | See [tss-futurerestore.md](docs/book/deep/tss-futurerestore.md) |
| `PURPLEPOIS0N_RECOVERY_UPLOAD` | Pre-signed iBSS/iBEC for Recovery upload | With `--gen0` in Recovery mode |
| `PURPLEPOIS0N_IM4M_MANIFEST` | IM4M for `ipsw img4 person` | Or extract via `--apticket` |

Build with **libtatsu** for in-tree live TSS (`make release` auto-detects; or `brew install libtatsu`).

If neither gaster nor ipwndfu is available, `-m` / `--checkm8` fails with a clear error (see `Checkm8.cpp`).

**Bundled ipsw (submodule):**

```bash
git submodule update --init external/ipsw
make external-ipsw   # builds external/ipsw/ipsw (Go required)
```

Analysis commands prefer `external/ipsw/ipsw`, then Homebrew `ipsw` on PATH. Export JSON for [boogeraids](docs/BOOGERAIDS.md) via `--analyze-json FILE`.

Generation 0 (greenpois0n / absinthe) is **not fully supported** — see [docs/SUPPORT.md](docs/SUPPORT.md).

### Examples

```bash
# List devices
./build/bin/purplepois0n -l

# checkm8 with verbose stage logging (DFU mode, gaster/ipwndfu required)
./build/bin/purplepois0n -v --checkm8

# Gen 0 scaffold with verbose logging
./build/bin/purplepois0n -v --gen0

# Analyze an on-disk iTunes backup (educational)
./build/bin/purplepois0n --analyze-backup ~/Library/Application\ Support/MobileSync/Backup/<udid>

# Target specific normal-mode device
./build/bin/purplepois0n -d 00008030-001A1D2E1E38802E
```

## Development

### Project Structure

```
purplepois0n/
├── include/           # Header files
│   ├── purplepois0n.h
│   ├── DeviceState.h
│   └── primitives/    # Primitive taxonomy (Bootrom, Kernel, Codesign, etc.)
├── src/               # Source files
│   ├── purplepois0n.cpp
│   ├── primitives/    # Transport adapters, registry, ChainRunner
│   ├── DeviceManager.{h,cpp}
│   ├── MobileDevice.{h,cpp}
│   ├── RecoveryDevice.{h,cpp}
│   ├── DFUDevice.{h,cpp}
│   ├── AFCService.{h,cpp}
│   ├── DyldCacheParser.{h,cpp}
│   ├── MachOParser.{h,cpp}
│   ├── MobileBackup.{h,cpp}
│   ├── Gen0Workflow.{h,cpp}
│   └── Logger.{h,cpp}
├── external/          # Git submodules (ipsw — blacktop/ipsw)
├── build/             # Build output directory
├── Makefile           # Build configuration
├── bootstrap.sh       # Dependency installation script
├── build.sh           # Alternative dependency script
├── docs/              # Lineage and jailbreak generation reference
│   ├── README.md
│   ├── LINEAGE.md
│   ├── GENERATIONS.md
│   └── book/          # Per-era educational chapters
└── README.md          # This file
```

### Adding Exploit Code

The framework uses a **primitive taxonomy** (`include/primitives/`) with category abstract classes (Bootrom, Kernel, Codesign, Sandbox, Injection) and operation types (Read, Write, Patch, Inject, Execute, Probe). Built-in primitives register via `PrimitiveRegistry`; `ChainRunner` orchestrates Detect → Connect → Probe → Report stages.

**Built-in probes (2026):** Gen6 chain modules (kfd, DarkSword, weightBufs, multicast_bytecopy, badRecovery, dmaFail, physrw, privilege, trustcache, bootstrap), historical stubs (limera1n, evasi0n, checkra1n), plus cross-gen probes: `Checkm8BootromPrimitive`, `OfflinePatchPrimitive`, `IpswdHostProbePrimitive`, `SandboxCapabilityProbePrimitive`, `AfcInjectionPrimitive`, `NormalModeProbePrimitive`, `BackupProbePrimitive`. Era-aware chains via `detectJailbreakGeneration()` / `runEraChain()`. See [docs/book/deep/primitives-gen0.md](docs/book/deep/primitives-gen0.md), [INTEGRATION_PLAN Phase 6–7](docs/legacy/INTEGRATION_PLAN.md), and [BACKPORT_MATRIX.md](docs/BACKPORT_MATRIX.md).

- Default builds are **probe-only** (no auto-pwn).
- Mutating primitives require `make plugins` (`PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS`) **and** CLI opt-in (`-m` for checkm8; `--gen0` execute path when mutation enabled).
- Gen 6 exploit modules delegate to external Dopamine `.framework` bundles via `dlopen` when `PURPLEPOIS0N_DOPAMINE_EXPLOITS` is set (`exploit_init` / `exploit_deinit`). Historical modules use `PURPLEPOIS0N_LIMERA1N`, `PURPLEPOIS0N_EVASI0N`, `PURPLEPOIS0N_CHECKRA1N`. Bootstrap stage can spawn `PURPLEPOIS0N_JB_HELPER`.
- Register new primitives with `PrimitiveRegistry::registerBuiltin()` and hook via `ChainRunner`.
- Gen 6 (Dopamine / PUAF): host prepares firmware offline (ipswd); exploit bytes stay **out of repo** (delegate only).

Integration points:

- Generation 0 scaffold: `src/Gen0Workflow.cpp` — `runGen0Jailbreak()` (DFU / Recovery / Normal)
- CLI entry: `src/purplepois0n.cpp` — `performJailbreak()` / `runDfuChain()`
- Umbrella header: `include/primitives/Primitives.h`

### API Usage Example

```cpp
#include "DeviceManager.h"
#include "Logger.h"
#include "DeviceState.h"
#include "DyldCacheParser.h"
#include "MachOParser.h"
#include "MobileBackup.h"

using namespace PP;

// Device detection
DeviceManager manager;
DeviceState state = manager.detectDeviceState();

if (state == DeviceState::Normal) {
    auto device = manager.getMobileDevice();
    Logger::info("Device: " + device->getDeviceName());
}

// Parse dyld cache
DyldCacheParser cache("/System/Library/Caches/com.apple.dyld/dyld_shared_cache_arm64");
if (cache.isValid()) {
    auto images = cache.getImageInfos();
    Logger::info("Found " + std::to_string(images.size()) + " images in cache");
}

// Parse Mach-O binary
MachOParser binary("/path/to/binary");
if (binary.isValid()) {
    auto segments = binary.getSegments();
    Logger::info("Found " + std::to_string(segments.size()) + " segments");
}

// Parse iOS backup
MobileBackup backup("/path/to/backup");
if (backup.isValid()) {
    auto files = backup.getAllFiles();
    Logger::info("Backup contains " + std::to_string(files.size()) + " files");
}
```

## Troubleshooting

### Build Issues

**Problem**: Libraries not found during linking
- **Solution**: Ensure dependencies are installed system-wide or update `LIBPATHS` in Makefile

**Problem**: Headers not found
- **Solution**: Check `INCLUDES` in Makefile and verify library installation paths

### Runtime Issues

**Problem**: "No device detected"
- **Solution**: 
  - Ensure device is connected via USB
  - Check USB cable and port
  - Verify device is unlocked (for normal mode)
  - Try different USB ports

**Problem**: Permission denied errors
- **Solution**: 
  - Add your user to the `plugdev` group: `sudo usermod -a -G plugdev $USER`
  - Log out and back in, or restart

## Legal Notice

This tool is for educational and research purposes only. Jailbreaking may:
- Void your device warranty
- Cause security vulnerabilities
- Violate terms of service
- Be illegal in some jurisdictions

Use at your own risk. The authors are not responsible for any damage or legal consequences.

## License

[Specify your license here]

## Contributing

[Contributing guidelines if applicable]

## Acknowledgments

- **Chronic Dev Team** — greenpois0n and absinthe, direct predecessors to this project’s lineage
- **libimobiledevice** team for device communication libraries used throughout the framework
- The broader **iOS jailbreak community** — evad3rs, Pangu, TaiG, checkra1n, unc0ver, Dopamine, and many others documented in [docs/GENERATIONS.md](docs/GENERATIONS.md)

## Author

posixninja

