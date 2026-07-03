# Bootstrap Chain & Kernel Patching Strategy

## Bootstrap Flow (Current)

```
┌─────────────────────────────────────────────────────────────────────┐
│ STAGE 0: BOOTROM (Read-Only, Hardware Level)                        │
├─────────────────────────────────────────────────────────────────────┤
│ • Device powers on with Hardware ROM code
│ • Loads BootROM security processor
│ • Performs chain-of-trust verification (Secure Boot)
│ • BootROM contains vulnerability (checkm8/usbliter8)
│ • purplepois0n exploits this → gets code execution in ROM context
└─────────────────────────────────────────────────────────────────────┘
                                ↓
         exploit runs via bootrom vulnerability
                                ↓
┌─────────────────────────────────────────────────────────────────────┐
│ STAGE 1: DFU MODE (After Bootrom Jailbreak)                         │
├─────────────────────────────────────────────────────────────────────┤
│ • Device enters Device Firmware Update (DFU) mode
│ • Device waits for host to send next stage (iBoot)
│ • Bootrom has been patched → will accept unsigned iBoot
│ • This is where we have full control over boot chain
│ • Syringe communication is active here
│
│ Decision Point:
│ ├─ Option A: Send NORMAL iBoot (continue normal flow)
│ ├─ Option B: Send PATCHED iBoot (with Cyanide hooks)
│ └─ Option C: Send MODIFIED iBoot (with ramdisk + Anthrax)
└─────────────────────────────────────────────────────────────────────┘
                                ↓
         [iBoot Selection & Loading via Syringe]
                                ↓
┌─────────────────────────────────────────────────────────────────────┐
│ STAGE 2: iBOOT EXECUTION                                            │
├─────────────────────────────────────────────────────────────────────┤
│ • iBoot (bootloader) runs from RAM
│ • Verifies kernel signature (can be patched out)
│ • Loads kernel into memory
│ • Loads ramdisk into memory
│ • Sets boot arguments (kernel parameters)
│
│ If Cyanide is active:
│ ├─ Cyanide patches are applied (via Syringe, in iBoot memory)
│ ├─ User gets interactive REPL for iBoot inspection
│ └─ Can modify boot arguments, kernel path, ramdisk path
│
│ If Anthrax is enabled:
│ ├─ iBoot patch: load MODIFIED ramdisk (not original)
│ ├─ iBoot patch: set boot arg for custom rootfs
│ └─ iBoot patch: disable various security checks
└─────────────────────────────────────────────────────────────────────┘
                                ↓
         [iBoot -> Kernel transition]
                                ↓
┌─────────────────────────────────────────────────────────────────────┐
│ STAGE 3: KERNEL BOOT                                                │
├─────────────────────────────────────────────────────────────────────┤
│ • iBoot jumps to kernel entry point
│ • Kernel initializes memory management, interrupts, etc.
│ • Kernel unpacks itself (KASLR - if enabled, already randomized)
│ • Mounts ramdisk as root filesystem
│ • Launches init process (launchd)
│
│ Kernel Patching at this stage:
│ ├─ Pre-boot patches: Already applied to kernelcache before load
│ └─ Runtime patches: Applied via ramdisk or kernel modules
└─────────────────────────────────────────────────────────────────────┘
                                ↓
         [Ramdisk mount & init execution]
                                ↓
┌─────────────────────────────────────────────────────────────────────┐
│ STAGE 4: RAMDISK (ANTHRAX) EXECUTION                                │
├─────────────────────────────────────────────────────────────────────┤
│ • Device now running userland
│ • Ramdisk contains SSH server, patch utilities, tools
│ • Root filesystem is Anthrax (custom ramdisk)
│ • Can optionally mount main OS (from flash storage)
│
│ Anthrax provides:
│ ├─ SSH server: Remote command execution
│ ├─ Mount tools: Access to main system
│ ├─ Patcher utilities: Live kernel patching tools
│ └─ Custom init: Can inject additional hooks
│
│ From here, user can:
│ ├─ SSH into device
│ ├─ Mount /dev/disk0s1s1 (main OS)
│ ├─ Run patch scripts with elevated privileges
│ └─ Dynamically modify kernel or filesystem
└─────────────────────────────────────────────────────────────────────┘
```

---

## Kernel Patching Strategies

We have **THREE distinct patching windows**:

### Strategy 1: Pre-Boot Patching (Offline)
```
IPSW File (Host)
    ↓
Extract KernelCache
    ↓
IpswPatchfinder finds patch sites
    ↓
Host applies patches to KernelCache binary
    ↓
Re-sign or disable signature verification
    ↓
Inject patched kernel into modified IPSW
    ↓
iBoot loads patched kernel
    ↓
Kernel boots with patches active
```

**Pros:**
- Patches active from kernel start
- No runtime overhead
- Clean patches before any security checks

**Cons:**
- Requires offline IPSW modification
- Must handle signature verification
- Less interactive/experimental

### Strategy 2: iBoot-Time Patching (Cyanide)
```
Device in DFU
    ↓
Load Cyanide payload via Syringe
    ↓
Cyanide REPL runs in iBoot context
    ↓
User issues patch commands:
    ├─ mem write 0x80001234 <patch-bytes>
    ├─ bp set 0x80002000  (breakpoint)
    └─ sym function_name  (resolve symbols)
    ↓
Patches applied to running iBoot
    ↓
User triggers boot, kernel loads with patches
```

**Pros:**
- Interactive experimentation
- No file I/O, pure memory patching
- Quick iteration cycles

**Cons:**
- Only patches iBoot, not kernel
- Patches lost on reboot
- Limited to what can be modified in iBoot

### Strategy 3: Live Ramdisk Patching (Anthrax)
```
Device boots with Anthrax ramdisk
    ↓
Kernel running, ramdisk is root
    ↓
Host SSH → Device
    ↓
Anthrax environment ready
    ↓
User executes patching scripts:
    ├─ launchctl stop com.apple.kernel-task
    ├─ kpf-patcher --apply-patch amfi_sandbox
    └─ launchctl start com.apple.kernel-task
    ↓
Patches applied to RUNNING kernel
    ↓
System continues with patches active
```

**Pros:**
- Live patching (no reboot)
- Can access filesystem for patch data
- Can verify patches took effect immediately

**Cons:**
- Kernel must be running (can't patch early boot)
- Complex memory manipulation
- Race conditions possible

---

## Implementation Strategy

### Current Bootstrap Implementation

In **purplepois0n.cpp**, the main() function:

```cpp
int main(int argc, char* argv[]) {
    // 1. Parse command line options
    Gen0CliOptions options = parseOptions(argc, argv);
    
    // 2. Initialize bootrom exploits
    initBootromExploits();  // Registers Checkm8, Usbliter8, Anthrax
    
    // 3. Create device manager (handles USB/device state)
    DeviceManager manager;
    
    // 4. List devices if requested
    if (options.listDevices) {
        listDevices(manager);
        return 0;
    }
    
    // 5. Detect and route to appropriate exploit
    if (!runBootromExploit(manager)) {
        Logger::error("Bootrom exploit failed");
        return 1;
    }
    
    // 6. Post-exploit hooks execute automatically
    //    - CyanidePayload (if --iboot-debug)
    //    - AnthraxPayload (if --anthrax-ramdisk)
    
    return 0;
}
```

**What's missing:**
1. Actual command-line parsing for --iboot-debug, --anthrax-ramdisk
2. Actual iBoot binary loading (currently stubbed)
3. Kernel cache extraction and patching (offline)
4. Ramdisk building and injection

### What Should Happen

**Option A: Interactive iBoot Debug Mode**
```bash
./purplepois0n --iboot-debug <IPSW>

Flow:
1. Device detection & CPID routing ✅
2. Run bootrom exploit ✅
3. Load Cyanide payload ✅
4. Cyanide REPL starts → User can:
   - dump 0x80000000 256
   - mem write 0x8000abcd <patch>
   - sym _start
   - quit
5. Device remains in DFU, ready for manual iBoot loading
```

**Option B: Full Boot with Ramdisk**
```bash
./purplepois0n --anthrax-ramdisk <IPSW> --boot

Flow:
1. Device detection & CPID routing ✅
2. Run bootrom exploit ✅
3. Build Anthrax ramdisk ❌ (TODO)
   - IpswPatchfinder discovers offsets
   - Extract ramdisk from IPSW
   - Inject SSH server
   - Inject patcher tools
4. Load modified iBoot ❌ (TODO)
5. iBoot boots kernel with Anthrax ramdisk
6. User SSH's into device
7. Live patching available
```

---

## Kernel Patching: Three-Tier Approach

```
┌─────────────────────────────────────────────────────────────┐
│ Tier 1: IpswPatchfinder (Discovery)                         │
├─────────────────────────────────────────────────────────────┤
│ Uses ipsw tool to find:                                      │
│ - Kernel entry point                                         │
│ - Boot arguments structure                                   │
│ - Patch sites (AMFI, sandbox, codesign, etc.)              │
│ - Function offsets across ALL devices                        │
│                                                              │
│ Output: KernelOffsets struct with all locations             │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ Tier 2: Patch Application (HOW TO PATCH)                    │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│ Choice A: Offline (iBoot before load)                       │
│   ├─ Extract kernelcache from KernelCache image             │
│   ├─ Decompress (LZSS or LZMA)                              │
│   ├─ Apply patches to binary at found offsets               │
│   ├─ Recompress                                             │
│   └─ Resign or disable codesigning check                    │
│                                                              │
│ Choice B: Online (Runtime via Syringe/Cyanide)             │
│   ├─ Load Cyanide payload in DFU                           │
│   ├─ Use Syringe.writeMemory() to patch iBoot              │
│   └─ Boot continues with patches active                    │
│                                                              │
│ Choice C: Post-Boot (Anthrax ramdisk)                       │
│   ├─ Device boots with Anthrax ramdisk                      │
│   ├─ SSH into device                                        │
│   ├─ Run patcher binary (uses kpf-purple framework)        │
│   └─ Patches applied to RUNNING kernel                     │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ Tier 3: Verification (DID IT WORK?)                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│ - Read back patched memory location                          │
│ - Verify new bytes match replacement pattern                │
│ - Check kernel behavior (sandbox disabled? codesign off?)   │
│ - Monitor system for side effects                            │
└─────────────────────────────────────────────────────────────┘
```

---

## Common Patch Categories

### AMFI (Apple Mobile File Integrity) Patches
**Purpose:** Disable code signature verification  
**Location:** In kernel's AMFI subsystem  
**Pattern (A5-A11):**
```
00 b1 00 24 20 46 90 bd  (set register to bypass check)
→
00 b1 01 24 20 46 90 bd  (modified to return success)
```
**Effect:** Any binary can run, even unsigned

### Sandbox Patches
**Purpose:** Disable sandbox restrictions  
**Location:** In kernel's sandbox policy engine  
**Pattern:** Various, depends on device/iOS version  
**Effect:** Apps can read/write anywhere on filesystem

### Codesign Patches
**Purpose:** Allow modified binaries  
**Location:** Code signing verification routines  
**Effect:** Can run patched system binaries

### Debug Enable
**Purpose:** Enable kernel debugging  
**Location:** Debug flags in iBoot/kernel  
**Pattern:** Set specific boot arguments  
**Effect:** Can use lldb, GDB, etc.

### tfp0 (task_for_pid 0)
**Purpose:** Allow arbitrary process memory access  
**Location:** Kernel syscall handler  
**Effect:** Userland can read/modify ANY process memory

---

## Current Missing Pieces

### 1. iBoot Binary Loading
```cpp
// Currently missing from AnthraxPayload::injectIntoBootChain()
// Needs:
// - Extract iBoot from IPSW
// - Decrypt iBoot (using IPSW keys)
// - Parse iBoot Mach-O to find patch sites
// - Apply iBoot patches
// - Send modified iBoot to device in DFU
// - Device loads it, exits DFU
```

### 2. Kernelcache Extraction & Patching
```cpp
// Needs implementation:
// - Extract KernelCache from IPSW
// - Decompress (LZSS or LZMA depending on iOS version)
// - Find kernel patches via IpswPatchfinder
// - Apply patches to kernelcache binary
// - Recompress
// - Inject back into IPSW or load directly
```

### 3. Ramdisk Building & SSH Injection
```cpp
// Currently stubbed in AnthraxPayload::buildRamdisk()
// Needs:
// - Extract ramdisk from IPSW (usually in iBoot or separate)
// - Decompress (usually CPIO)
// - Add SSH server binary (dropbear or openssh)
// - Add SSH keys for authentication
// - Add patcher utilities (kpf framework)
// - Recompress
// - Make bootable
```

### 4. Live Kernel Patching (Anthrax Runtime)
```cpp
// Needs implementation:
// - SSH into Anthrax ramdisk
// - Run patcher binary
// - Patcher uses kpf-purple framework
// - Searches running kernel for patch patterns
// - Applies patches to running kernel
// - Verifies patches took effect
```

---

## Bootstrap Entry Points

### Entry Point 1: `initBootromExploits()`
**Where:** `src/purplepois0n.cpp:209`  
**What:** Registers Checkm8, Usbliter8, and Anthrax hooks  
**Missing:** Should also initialize Syringe singleton

### Entry Point 2: `BootromExploitRegistry::runForDevice()`
**Where:** `src/BootromExploit.cpp:49`  
**What:** CPID detection → exploit routing → post-exploit execution  
**Current:** Works correctly ✅

### Entry Point 3: `AnthraxExploit::run()`
**Where:** `src/AnthraxExploit.cpp`  
**What:** Triggers post-exploit payload setup  
**Missing:** Should orchestrate ramdisk building + iBoot patching

### Entry Point 4: `CyanidePayload::load()`
**Where:** `src/CyanidePayload.cpp:17`  
**What:** Initializes iBoot debugger  
**Current:** Initializes Syringe ✅, but Cyanide REPL stub

---

## Recommended Bootstrap Order

To make this system fully functional:

### Phase 1: Core Infrastructure (DONE ✅)
- BootromExploit registry and routing
- Syringe USB abstraction
- Cyanide/Anthrax payload classes
- IpswPatchfinder discovery

### Phase 2: Implement Syringe Details (NEXT)
- `doDFURead()` - Memory read via irecovery
- `doDFUWrite()` - Memory write via irecovery
- DFU device handling
- Request/response serialization

### Phase 3: Implement Cyanide REPL (NEXT)
- parseCommand() - Already done ✅
- executeCommand() - Memory ops using Syringe ✅
- runREPL() - Main loop ✅
- Test with actual device in DFU

### Phase 4: Implement IpswPatchfinder (NEXT)
- Integrate ipsw CLI tool
- Parse ipsw analyze output
- Extract kernel offsets automatically
- Match patch patterns across devices

### Phase 5: Implement Anthrax Ramdisk (NEXT)
- Extract ramdisk from IPSW
- Inject SSH server
- Inject patch utilities
- Build bootable ramdisk image

### Phase 6: Integration Testing
- End-to-end device boot
- Cyanide REPL on real device
- Anthrax SSH access
- Live kernel patching via ramdisk

---

## Example: Complete Patching Flow

```bash
User runs:
$ ./purplepois0n --anthrax-ramdisk iPhone_6S.ipsw --patch-sandbox --patch-amfi --boot

Execution flow:

1. Detect device in DFU
   └─ CPID 0x8974 → Checkm8Exploit selected

2. Run Checkm8
   └─ Device jailbroken at ROM level

3. Initialize post-exploit hooks
   ├─ CyanidePayload::load() → Syringe initialized
   └─ AnthraxExploit::run()

4. AnthraxExploit builds ramdisk
   ├─ IpswPatchfinder::discoverOffsets()
   │  ├─ Extract kernelcache from IPSW
   │  ├─ Run: ipsw analyze kernelcache.decrypted
   │  └─ Output: AMFI offset, sandbox offset, etc.
   ├─ AnthraxPayload::buildRamdisk()
   │  ├─ Extract ramdisk.cpio from IPSW
   │  ├─ injectSSHTools()
   │  ├─ injectPatchers()
   │  └─ Recompress to ramdisk.patched
   └─ AnthraxPayload::injectIntoBootChain()
      ├─ Extract iBoot from IPSW
      ├─ Patch iBoot to load ramdisk.patched
      ├─ Load patched iBoot to device via Syringe
      └─ Device boots normally (exits DFU)

5. Device boots with Anthrax ramdisk
   ├─ iBoot loads patched ramdisk
   ├─ Kernel starts
   └─ Anthrax init runs

6. User SSH into device
   $ ssh root@192.168.1.100

7. Anthrax patcher runs (sandboxed inside Anthrax)
   device# /usr/local/bin/kpf-patcher \
     --patch-sandbox at-offset-0x12345 \
     --patch-amfi at-offset-0x67890
   
   └─ Patches applied to RUNNING kernel

8. Verify patches worked
   device# launchctl start com.apple.springboard
   └─ Sandbox bypass confirmed

Device now has full root access with all security patches disabled.
```

This architecture allows **complete customization at every boot stage**.

---

## Security Model

⚠️ **Bootstrap Security Considerations:**

1. **Bootrom Level** (Stage 0)
   - ROM is read-only, vulnerability can't be patched
   - Device will always be vulnerable to checkm8/usbliter8
   - Only solution: new hardware with patched ROM

2. **iBoot Level** (Stage 2)
   - iBoot is loaded to RAM, can be modified
   - Signature verification can be patched out
   - Secure Boot can be disabled via boot arguments

3. **Kernel Level** (Stage 3)
   - Kernel is loaded with patches active
   - All security systems can be patched
   - Code signing, sandbox, all disabled

4. **Ramdisk Level** (Stage 4)
   - Ramdisk is root, has complete control
   - Can mount main OS, modify system files
   - Patches persist only while ramdisk is active

**Result:** Complete jailbreak with no security measures active.

