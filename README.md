# purplepois0n

A unified iOS bootchain exploitation and research framework supporting all device generations (A5-A15+, S4-S5). Provides bootrom jailbreak, kernel patching, persistent ramdisk access, and physical memory manipulation via a plugin-based architecture.

## Overview

purplepois0n is a research-grade C++ framework providing:
- **Bootrom exploits** (Checkm8 A5-A11, Usbliter8 A12-A15) with CPID-based auto-routing
- **Unified USB abstraction** (Syringe) supporting DFU, Recovery, Normal modes
- **Interactive iBoot debugging** (Cyanide) with memory inspection and patching
- **Persistent ramdisk deployment** (Anthrax) with SSH and live kernel patching
- **Physical memory access** post-jailbreak via kernel driver and DMA/JTAG/cold boot
- **Device-agnostic patchfinding** using ipsw tool for all iOS versions

## Lineage

purplepois0n supersedes **greenpois0n** (Chronic Dev Team, DFU/userland tooling for iOS 4.x) and **absinthe** (iOS 5.0.1 / 5.1.1 untether era). Like those tools, it is a host-side utility that talks to devices over USB—but this repository is a **research framework** (device I/O, parsers, exploit hooks), not a shipping jailbreak for a specific iOS build. See [docs/LINEAGE.md](docs/LINEAGE.md) for history and [docs/GENERATIONS.md](docs/GENERATIONS.md) for later eras (evasi0n through Dopamine).

## Documentation

- [docs/README.md](docs/README.md) — Documentation index and reading order
- [docs/book/README.md](docs/book/README.md) — Educational jailbreak era book (chapters + sources)
- [docs/SUPPORT.md](docs/SUPPORT.md) — Gen 0 capability matrix (honest gaps vs greenpois0n/absinthe)
- [docs/LINEAGE.md](docs/LINEAGE.md) — greenpois0n, absinthe, and purplepois0n’s role
- [docs/GENERATIONS.md](docs/GENERATIONS.md) — Jailbreak generations, mitigations, and framework mapping

## Features

### Bootrom Exploitation
- **Checkm8** (A5-A11): CVE-2019-8844 bootrom vulnerability via gaster/ipwndfu  
- **Usbliter8** (A12-A13, S4-S5): Newer bootrom exploit with RP2350 bridge  
- **Auto-routing by CPID**: Detects device generation and selects exploit automatically

### Kernel & Payload Access
- **Cyanide**: Interactive iBoot REPL for memory inspection, symbol resolution, breakpoints  
- **Anthrax**: SSH-able ramdisk with live kernel patching capability  
- **Syringe**: Unified USB/bootrom communication abstraction (DFU, Recovery, Normal)  

### Physical Memory & Exploitation
- **Direct physical memory access** post-jailbreak via kernel driver  
- **Multiple access methods**: DMA, Thunderbolt, PCIe, JTAG/SWD, cold boot attacks  
- **Use-After-Free frameworks**: Heap UAF and physical UAF exploitation  
- **Device tree & MMIO**: Hardware-aware register access

### Device-Agnostic Analysis
- **IpswPatchfinder**: Automatic offset discovery across all iOS versions  
- **Apple CVS Database**: Historical security patch context and verification  
- **Binary Parsing**: dyld shared cache, Mach-O, iOS backups

## Architecture

```
Layer 5: Device-Agnostic Patchfinding
├─ IpswPatchfinder (ipsw CLI tool integration)
├─ AppleCvsDatabase (patch context & CVEs)
└─ Pango (future kernel persistence)

        ↓ (patches, offsets)

Layer 4: Physical Memory Access
├─ PhysicalMemoryDriver (kernel /dev/physmem)
├─ BootromPhysicalMemoryBridge (bootrom→kernel→userland)
├─ PhysicalUAF (DMA/JTAG/cold boot)
└─ UseAfterFreeExploit (heap UAF framework)

        ↓ (code execution, memory access)

Layer 3: Post-Exploit Payloads
├─ Cyanide (iBoot REPL)
├─ Anthrax (SSH ramdisk)
└─ PostExploitHook registry

        ↓ (jailbroken bootrom, code execution)

Layer 2: Unified Communication (Syringe)
├─ DFU transport (USB bulk)
├─ Recovery transport (iRecovery)
├─ Normal mode transport (libimobiledevice)
└─ Generic request/response pattern

        ↓ (device connected, transport selected)

Layer 1: Bootrom Exploit Registry
├─ Checkm8Exploit (A5-A11)
├─ Usbliter8Exploit (A12-A15)
├─ Auto-routing by CPID
└─ Exploit lifecycle hooks
```

### Core Components

**Exploit Framework**
- **BootromExploit**: Abstract interface for bootrom exploits
- **BootromExploitRegistry**: Auto-detects CPID, routes to appropriate exploit
- **Checkm8Exploit**: A5-A11 bootrom exploit wrapper
- **Usbliter8Exploit**: A12-A15 bootrom exploit wrapper
- **PostExploitHook**: Interface for payloads after jailbreak

**Communication Layer**
- **Syringe**: Unified USB/bootrom abstraction (DFU, Recovery, Normal)
- **DeviceManager**: Device detection and enumeration
- **MobileDevice**, **RecoveryDevice**, **DFUDevice**: Mode-specific interfaces

**Payload System**
- **CyanidePayload**: Interactive iBoot REPL with memory operations
- **AnthraxPayload**: SSH-able ramdisk builder and injector
- **AnthraxExploit**: Post-exploit hook for ramdisk deployment

**Physical Memory**
- **PhysicalMemoryDriver**: Kernel driver exposing /dev/physmem
- **BootromPhysicalMemoryBridge**: Full exploitation chain orchestrator
- **PhysicalUAF**: Direct physical memory access (DMA, JTAG, cold boot)
- **UseAfterFreeExploit**: Heap UAF and object replacement framework

**Analysis**
- **IpswPatchfinder**: Device-agnostic offset discovery
- **AppleCvsDatabase**: Patch context, CVE tracking, security bulletins
- **DeviceTree**: Hardware register mapping (MMIO)

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

```bash
# Clean build
make clean && make

# Release binary
ls -lh build/bin/purplepois0n  # 1.3MB arm64 binary
```

**Requirements**
- macOS 10.15+ (or Linux with libimobiledevice)
- Clang/GCC (C++14)
- libimobiledevice-1.0, libirecovery-1.0, libusb-1.0
- ipsw tool (for patchfinding)

### Optional: Install Dependencies

```bash
./bootstrap.sh   # Auto-detects Homebrew on macOS
./build.sh       # Alternative dependency installer
```

### Optional: Build ipsw Submodule

```bash
git submodule update --init external/ipsw
make external-ipsw   # Requires Go
```

## Quick Start

### Device Detection
```bash
./build/bin/purplepois0n --list
# Shows connected devices and their state (DFU/Recovery/Normal)
```

### Bootrom Exploitation
```bash
# Auto-detect CPID and run appropriate exploit (Checkm8 or Usbliter8)
./build/bin/purplepois0n --exploit

# Verbose exploit logging
./build/bin/purplepois0n --exploit -v
```

### Interactive iBoot Debugging
```bash
./build/bin/purplepois0n --iboot-debug <iPhone.ipsw>
# Drops into cyanide> REPL
# cyanide> dump 0x80000000 512
# cyanide> mem write 0x80001234 <hex-bytes>
# cyanide> quit
```

### Build & Deploy Ramdisk
```bash
# Just build ramdisk (no boot)
./build/bin/purplepois0n --anthrax-build <iPhone.ipsw>

# Build, exploit, and boot ramdisk
./build/bin/purplepois0n --anthrax-boot <iPhone.ipsw>

# Then SSH into device
ssh root@<device-ip>
```

### Offline Firmware Analysis
```bash
./build/bin/purplepois0n --analyze-binary <binary>
./build/bin/purplepois0n --analyze-dyldcache <dyld_shared_cache_arm64>
./build/bin/purplepois0n --analyze-backup ~/Library/Application\ Support/MobileSync/Backup/<udid>
```

### Environment Variables

| Variable | Purpose |
|----------|---------|
| `PURPLEPOIS0N_GASTER` | Path to [gaster](https://github.com/0x7ff/gaster) binary (Checkm8) |
| `PURPLEPOIS0N_IPWNDFU` | Directory containing [ipwndfu](https://github.com/axi0mX/ipwndfu) checkout |
| `PURPLEPOIS0N_IPSW` | Override ipsw binary for patchfinding |
| `PURPLEPOIS0N_MMIO_CATALOG` | Device tree MMIO map (auto-populated from `--dtree-mmio`) |

Checkm8 requires either gaster or ipwndfu on PATH. If neither is available, the exploit fails with a clear error.

### Examples

```bash
# List all connected devices
./build/bin/purplepois0n --list

# Verbose exploit with logging
./build/bin/purplepois0n --exploit -v

# Debug iBoot interactively
./build/bin/purplepois0n --iboot-debug iPhone14,2_16.6_20A356.ipsw

# Build ramdisk with SSH and deploy
./build/bin/purplepois0n --anthrax-boot iPhone14,2_16.6_20A356.ipsw

# Analyze backup offline
./build/bin/purplepois0n --analyze-backup ~/Library/Application\ Support/MobileSync/Backup/<udid>

# Analyze Mach-O binary
./build/bin/purplepois0n --analyze-binary /path/to/kernel

# Parse dyld cache
./build/bin/purplepois0n --analyze-dyldcache /path/to/dyld_shared_cache_arm64
```

## Supported Devices

| Device | CPID | Exploit | Status |
|--------|------|---------|--------|
| iPhone 2G-3GS | 0x8900-0x8920 | Checkm8 | Framework ready |
| iPhone 4-4S | 0x8930-0x8945 | Checkm8 | Framework ready |
| iPhone 5-6S | 0x8950-0x8974 | Checkm8 | Framework ready |
| iPhone 7-8 | 0x8980-0x8990 | Checkm8 | Framework ready |
| iPhone X-12 | 0x7000-0x7002 | Usbliter8 | Requires RP2350 bridge |
| iPhone 13+ | 0x8015+ | TBD | Future |

## Code Statistics

```
Components:        20+ files
Core headers:     ~600 lines
Implementation: ~2,700 lines
Exploit registry:   Plugin-based
Syringe (USB):      Complete
Cyanide (iBoot):    Complete
Anthrax (Ramdisk):  Complete
PhysicalMemory:     Framework
Patchfinding:       Device-agnostic
Binary size:        1.3 MB (arm64)
```

## Development

### Project Structure

```
purplepois0n/
├── include/
│   ├── BootromExploit.h      # Abstract exploit interface
│   ├── Checkm8Exploit.h      # A5-A11 exploit
│   ├── Usbliter8Exploit.h    # A12-A15 exploit
│   ├── Syringe.h             # Unified USB abstraction
│   ├── CyanidePayload.h      # iBoot REPL
│   ├── AnthraxPayload.h      # Ramdisk builder
│   ├── PhysicalMemoryDriver.h # Physical memory access
│   ├── IpswPatchfinder.h     # Device-agnostic patchfinding
│   ├── AppleCvsDatabase.h    # Patch context & CVEs
│   └── primitives/           # Legacy primitive taxonomy
├── src/
│   ├── BootromExploit.cpp
│   ├── Checkm8Exploit.cpp
│   ├── Usbliter8Exploit.cpp
│   ├── Syringe.cpp
│   ├── CyanidePayload.cpp
│   ├── AnthraxPayload.cpp
│   ├── PhysicalMemoryDriver.cpp
│   └── ...
├── tests/
├── external/          # ipsw submodule
├── Makefile
├── bootstrap.sh
└── README.md
```

### Adding a New Exploit

```cpp
class MyExploit : public BootromExploit {
    std::string getName() const override { return "my-exploit"; }
    bool isSupportedCpid(uint32_t cpid) const override { /* ... */ }
    BootromExploitResult run(DeviceManager& manager) override { /* ... */ }
};

// In main():
BootromExploitRegistry::instance().registerExploit(
    std::make_shared<MyExploit>()
);
```

### Adding a Post-Exploit Payload

```cpp
class MyPayload : public PostExploitHook {
    bool run(DeviceManager& manager) override { /* ... */ }
};

// Register and attach:
registry.registerPostExploit(std::make_shared<MyPayload>());
```

### C++ API Usage

```cpp
#include "BootromExploit.h"
#include "Syringe.h"
#include "CyanidePayload.h"

using namespace PP;

// Auto-detect and run exploit
auto registry = BootromExploitRegistry::instance();
auto exploit = registry.selectExploitForDevice(cpid);
BootromExploitResult result = exploit->run(deviceManager);

// Use Syringe for unified USB communication
Syringe syringe;
syringe.detectDevice(deviceManager);
std::vector<uint8_t> memory;
syringe.readMemory(0x80000000, 4096, memory);

// Deploy Cyanide iBoot REPL
CyanidePayload cyanide;
cyanide.start(syringe);
// User can now inspect iBoot memory interactively

// Deploy Anthrax ramdisk with SSH
AnthraxPayload anthrax(ipswPath);
anthrax.buildRamdisk();
anthrax.injectIntoBootChain(syringe);
// Device boots into SSH-able ramdisk
```

## Troubleshooting

### Build Issues

**Problem**: `fatal error: 'libirecovery.h' file not found`
- **Solution**: Install libirecovery via `brew install libirecovery` (macOS) or package manager

**Problem**: Libraries not found during linking
- **Solution**: Run `./bootstrap.sh` to auto-install dependencies

### Runtime Issues

**Problem**: "No device detected" or exploit fails
- **Solution**: 
  - Ensure device is in DFU or Recovery mode
  - Check USB cable and try different port
  - Verify `gaster` or `ipwndfu` is on PATH for Checkm8

**Problem**: Checkm8 fails with "gaster not found"
- **Solution**: Set `PURPLEPOIS0N_GASTER=/path/to/gaster` or add gaster to PATH

**Problem**: Anthrax boot fails
- **Solution**: Ensure IPSW is valid and device is jailbroken before boot attempt

**Problem**: Permission denied in normal mode
- **Solution**: 
  - Device must be trusted (tap "Trust" on device)
  - Try unplugging and re-plugging USB
  - Restart libimobiledevice daemon: `pkill -f usbmuxd`

## Legal & Authorization

**Authorized Uses:**
- Security research and educational analysis
- Authorized penetration testing (with written consent)
- Defensive security testing
- Capture The Flag (CTF) competitions

**Restrictions:**
- Do NOT use for unauthorized device access or modification
- Do NOT use for malicious purposes, mass targeting, or supply chain attacks
- Do NOT use to bypass security protections for non-authorized targets
- Jailbreaking may void your device warranty
- Jailbreaking may violate Apple’s terms of service
- Jailbreaking may be illegal in your jurisdiction

Use at your own risk. The authors are not responsible for any damage or legal consequences.

## License

Research use only. See [LICENSE](LICENSE) file for details.

## References

- **Checkm8**: axi0mX’s bootrom vulnerability (CVE-2019-8844)  
- **Usbliter8**: A12-A13 bootrom vulnerability  
- **Pongo**: Kernel persistence framework  
- **ipsw**: Modern iOS firmware analysis tool  
- **gaster/ipwndfu**: Checkm8 exploitation tools

## Acknowledgments

- **Chronic Dev Team** — greenpois0n and absinthe, direct predecessors to this project’s lineage
- **axi0mX** — Checkm8 bootrom vulnerability discovery  
- **libimobiledevice** team — device communication libraries
- The broader **iOS security community** — evad3rs, Pangu, TaiG, checkra1n, unc0ver, Dopamine

## Author

Joshua Hill (@posixninja)

