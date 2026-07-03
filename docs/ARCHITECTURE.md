# Purplepois0n Complete Architecture

## Overview

Purplepois0n is a **unified bootrom-to-kernel experimental framework** that combines:
- **Bootrom exploits** (checkm8, usbliter8) with auto-routing by device
- **iBoot payload** (cyanide) for interactive debugging and patching
- **Ramdisk environment** (anthrax) with SSH access for live system manipulation
- **Device-agnostic patchfinding** (ipsw integration) across all device generations
- **Unified USB abstraction** (syringe) for all communication

### Design Philosophy

This is not a traditional "one-click jailbreak." Instead, it's an **educational platform and research toolkit** that:

1. Teaches the iOS bootchain (ROM → iBoot → Kernel → Userland)
2. Demonstrates historical and modern exploitation techniques
3. Allows developers to experiment with kernel patching
4. Provides extensible hooks for new exploits and payloads
5. Works uniformly across A5-A13+, S4-S5 devices without hardcoded offsets

---

## Architecture Layers

```
┌─────────────────────────────────────────────────────────────┐
│  Application Layer (Gen0Workflow, CLI)                      │
├─────────────────────────────────────────────────────────────┤
│  Bootrom Exploit Registry                                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Bootrom Detection (CPID → Exploit Router)            │   │
│  │  - Checkm8 (A5-A11, no hardware)                     │   │
│  │  - Usbliter8 (A12/A13/S4/S5, needs RP2350)          │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  Post-Exploit Payload Hooks                                 │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Cyanide (iBoot Debugger)      │  Anthrax (Ramdisk)  │   │
│  │ - REPL for iBoot inspection   │  - SSH-able         │   │
│  │ - Memory read/write           │  - Mount main OS    │   │
│  │ - Breakpoint support          │  - Live patching    │   │
│  │ - iBoot patching              │  - Execute commands │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  Unified Communication Layer (Syringe)                      │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Transport Abstraction                                │   │
│  │  DFU ↔ Recovery ↔ Normal Mode                        │   │
│  │ Memory Operations, Code Execution, File Transfer     │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  Patchfinding (IpswPatchfinder)                             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ ipsw Tool Integration                                │   │
│  │ - Device-agnostic offset discovery                   │   │
│  │ - Kernel entry point finding                         │   │
│  │ - Function symbol resolution                         │   │
│  │ - Patch site identification                          │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  Hardware Transports                                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ libusb (DFU) │ libimobiledevice │ irecovery         │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## Execution Flow

### 1. **Device Detection & Bootrom Exploit**

```
Connect Device (USB)
    ↓
Detect Device State
    ├─→ DFU Mode → Ready for exploit
    ├─→ Recovery Mode → Transition to DFU
    └─→ Normal Mode → Restart to Recovery → DFU
    ↓
Query CPID via Syringe
    ↓
BootromExploitRegistry Routes by CPID
    ├─→ 0x8960+ (A5-A11) → Checkm8Exploit
    └─→ 0x7000+ (A12-A13, S4-S5) → Usbliter8Exploit
    ↓
Execute Bootrom Exploit
    ↓
SUCCESS: Device remains in DFU, now jailbroken at ROM level
```

### 2. **Post-Exploit: Cyanide iBoot Debugger**

```
Bootrom Exploit Complete
    ↓
Initialize Syringe (DFU transport)
    ↓
Load Cyanide Payload
    ├─→ Initialize iBoot communication
    ├─→ Start REPL command loop
    └─→ Enable memory inspection
    ↓
User enters Cyanide REPL commands:
    ├─→ mem read 0x80000000 256     (Read iBoot memory)
    ├─→ mem write 0x80001234 [...]  (Patch iBoot)
    ├─→ bp set 0x80002000           (Set breakpoint)
    ├─→ sym _start                  (Resolve symbol)
    └─→ exit
    ↓
Device still in DFU, iBoot patched
```

### 3. **Post-Exploit: Anthrax Ramdisk**

```
Bootrom/iBoot Patching Complete
    ↓
Build Anthrax Ramdisk
    ├─→ Use ipsw to locate ramdisk in IPSW
    ├─→ Inject SSH server (dropbear/OpenSSH)
    ├─→ Inject patcher utilities
    └─→ Recompress ramdisk
    ↓
Inject into Boot Chain
    ├─→ Patch iBoot to load modified ramdisk
    └─→ Set boot arguments for ramdisk
    ↓
Boot Kernel
    ├─→ iBoot loads patched ramdisk
    ├─→ Kernel starts
    └─→ Ramdisk mounts as rootfs
    ↓
Device now running SSH-able Anthrax ramdisk
    ↓
SSH into Device (from host)
    ├─→ ssh root@<device-ip>
    ├─→ mount /dev/disk0s1s1 /mnt  (Mount main OS)
    ├─→ /mnt/bin/launchctl ...      (Execute live patches)
    └─→ exit
    ↓
Kernel is live-patched via ramdisk
```

---

## Key Components

### **BootromExploit Interface**
- Abstract base class for all bootrom exploits
- Implementations: Checkm8Exploit, Usbliter8Exploit
- Plugin registry pattern for extensibility
- Post-exploit hooks for payloads (Cyanide, Anthrax)

### **Syringe (Unified Communication)**
- Abstracts DFU, Recovery, Normal mode
- Provides: Memory R/W, code execution, file transfer
- Used by all payloads for device communication
- Request/response pattern for logging and debugging

### **CyanidePayload (iBoot Debugger)**
- Interactive REPL for iBoot manipulation
- Commands: mem, bp (breakpoint), patch, sym (symbol), dump, quit
- Reads/writes iBoot memory via Syringe
- Enables interactive experimentation with iBoot

### **AnthraxPayload (Ramdisk Environment)**
- Builds SSH-able ramdisk from IPSW
- SSH connectivity for remote command execution
- Main filesystem mounting for live patching
- Extends kernel persistence beyond boot

### **IpswPatchfinder (Device-Agnostic Discovery)**
- Uses ipsw tool to analyze any IPSW
- Finds kernel entry, boot args, function offsets
- Identifies patch sites (sandbox, code signing, etc.)
- Works uniformly: A5 through A13+, S4-S5

---

## Data Flow Example: Live Patching

```
User runs: ./purplepois0n --iboot-debug --anthrax-ramdisk <IPSW>

1. Device Detection
   Device (USB) → Syringe.detectDevice() → DFU

2. CPID Routing
   Syringe.queryCpid() → 0x8920 → Checkm8Exploit

3. Bootrom Exploit
   Checkm8Exploit.run(manager) → Device jailbroken at ROM

4. Cyanide Hook
   AnthraxExploit.run() → Start CyanidePayload → REPL
   User: "dump 0x80001000 0x100" → Syringe.readMemory() → Output

5. Ramdisk Building
   IpswPatchfinder.discoverOffsets() → Parse ipsw
   AnthraxPayload.buildRamdisk() → Create ramdisk
   AnthraxPayload.injectIntoBootChain() → Patch iBoot

6. Boot with Ramdisk
   Reboot device → iBoot loads patched ramdisk
   Anthrax starts, SSH server ready

7. Live Patching
   Host SSH → Device
   Device: mount /dev/disk0s1s1 /mnt
   Device: /usr/bin/patch-kernel  # Custom tool
   Device: launchctl stop com.apple.springboard  # Kill app
   User has live control
```

---

## Extension Points

### Adding a New Bootrom Exploit
```cpp
class MyExploit : public BootromExploit {
    std::string getName() const override { return "my-exploit"; }
    bool isSupportedCpid(uint32_t cpid) const override { /* ... */ }
    BootromExploitResult run(DeviceManager& manager) override { /* ... */ }
};

// Register in purplepois0n.cpp:
registry.registerExploit(std::make_shared<MyExploit>());
```

### Adding a New Post-Exploit Payload
```cpp
class MyPayload : public PostExploitHook {
    bool run(DeviceManager& manager) override { /* ... */ }
};

// Register in purplepois0n.cpp:
registry.registerPostExploit(std::make_shared<MyPayload>());
```

### Adding Memory Operations via Syringe
```cpp
// Existing code automatically uses:
std::vector<uint8_t> buffer;
syringe->readMemory(0x80000000, 0x1000, buffer);
syringe->writeMemory(0x80001234, patches);
```

---

## Supported Devices

| Device | CPID | Exploit | RAM | Notes |
|--------|------|---------|-----|-------|
| iPhone 2G-3GS | 0x8900-0x8920 | Checkm8 | 128-256MB | Original devices |
| iPhone 4-4S | 0x8930-0x8945 | Checkm8 | 512MB-1GB | A4-A5 |
| iPhone 5-6S | 0x8950-0x8974 | Checkm8 | 1-2GB | A6-A9 |
| iPhone 7-8 | 0x8980-0x8990 | Checkm8 | 2-3GB | A10-A11 |
| iPhone X-12 | 0x7000-0x7002 | Usbliter8 | 3-6GB | A12-A15, needs RP2350 |
| iPhone 13+ | 0x8015+ | TBD | 4GB+ | Newer CPIDs |
| iPad Air-Pro | 0x8960-0x7001 | Checkm8/Usbliter8 | Varies | Device-specific |

**Note**: All devices work with IpswPatchfinder regardless of CPID.

---

## Usage Examples

### Basic Device Detection
```bash
./purplepois0n --list
# Output: Device 1: iPhone 6S (CPID: 0x8974, DFU mode)
```

### Interactive iBoot Debugging
```bash
./purplepois0n --iboot-debug <IPSW>
# Drops into cyanide> REPL
# cyanide> dump 0x80000000 512
# cyanide> mem write 0x80001234 [bytes...]
# cyanide> quit
```

### Build Anthrax Ramdisk
```bash
./purplepois0n --build-anthrax <IPSW> --output anthrax.img
# Generates SSH-able ramdisk ready for injection
```

### Full Boot with Modifications
```bash
./purplepois0n --iboot-debug --anthrax-ramdisk --boot <IPSW>
# Interactive iBoot debugging, then boots with modified ramdisk
```

---

## Technical Highlights

### ✅ Device-Agnostic
- No hardcoded offsets (uses ipsw patchfinding)
- Supports A5-A15+ automatically
- Works across all variants (WiFi, Cellular, etc.)

### ✅ Educational
- Each layer is documented and testable independently
- Cyanide REPL teaches iBoot structure
- Anthrax demonstrates kernel manipulation
- Source code is clear and approachable

### ✅ Extensible
- Plugin architecture for new exploits
- Post-exploit hook system for new payloads
- Generic Syringe layer for device communication

### ✅ Modern
- Support for Secure Boot (A12+)
- ARM64 PAC (Pointer Authentication) awareness
- Modern entropy/ASLR considerations
- Ramdisk-based persistence patterns

---

## Building & Compilation

```bash
make clean
make

# Output: build/bin/purplepois0n (900KB arm64)
```

**Requirements:**
- macOS 10.15+ (or Linux with libimobiledevice)
- clang/g++ (C++14)
- libimobiledevice-1.0
- lirecovery-1.0
- libusb-1.0
- ipsw tool (for patchfinding)

---

## Security Considerations

This tool is designed for **authorized experimentation and education only**:

✅ Legitimate use:
- Security research on own devices
- Educational bootchain analysis
- CTF competitions
- Understanding Apple's bootrom security

❌ Do not use for:
- Unauthorized device jailbreaking
- Bypassing security for malicious purposes
- Distributing pirated software

All modifications are local to the device and require physical USB access.

---

## Future Directions

1. **Hardware Support**: Extend usbliter8 beyond A12/A13/S4/S5
2. **Kernel Patching Helpers**: Build library of common kernel patches
3. **Pongo Integration**: Unified Pongo/Cyanide payload system
4. **Documentation**: Detailed bootrom/iBoot exploit walkthroughs
5. **Tutorials**: Step-by-step experiments for learning

---

## References

- Checkm8: axi0mX's bootrom vulnerability (CVE-2019-8844)
- Usbliter8: A12-A13 bootrom vulnerability
- Syringe: Unified USB/bootrom abstraction (inspired by qilin)
- Cyanide: Legacy iBoot debugger (Chronic-Dev GreenPois0n era)
- Anthrax: Ramdisk-based persistence framework
- ipsw tool: Modern iOS firmware analysis tool

---

**Version**: 1.0 (Unified Architecture)  
**Author**: purplepois0n team  
**License**: Research use only
