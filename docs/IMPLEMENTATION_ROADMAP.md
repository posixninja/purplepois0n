# Implementation Roadmap

> **MVP (scan → plan → execute):** see [MVP.md](MVP.md) and [validation/mvp-smoke.md](validation/mvp-smoke.md). Post-MVP work below.

## MVP remainder (priority)

| Item | Status | Notes |
|------|--------|-------|
| Web wizard + `/device/plan` | Done | JailbreakWizard plan card + auto doctor |
| `JailbreakPlanner` + IPSW ramdisk auto | Done | `PURPLEPOIS0N_IPSW`, blockers in plan JSON |
| Generic boot delivery | Done | `BootChain`, `UsbLoaderBootChainPrimitive` |
| MVP docs + store journey | Done | `MVP.md`, `DOCTOR.md`, `STORE_ECOSYSTEM.md` MVP section |
| `make smoke-mvp` aggregate | Done | kpf, dfu, store, doctor, agent, device-plan, web-build |
| `device plan` subcommand | Done | Rewrites to `--device-plan` |
| Gen6 in-tree full execute | Partial | Probe chain; not parity with Dopamine |
| Normal `/var/jb` detection via SSH in planner | Done | Agent sets `PURPLEPOIS0N_NORMAL_SSH`; planner uses SSH transport |
| Doctor execute + store sync for already-jailbroken | Done | Planner `postJbStoreSync` + merge SSH connect |
| Agent API reference doc | Done | [AGENT_API.md](AGENT_API.md) |
| Live DFU E2E on hardware | Manual | Needs device + plugins + IPSW |
| In-tree LLM agent | Out of scope | Use agent HTTP JSON; external LLM optional |

---

## Current Status

✅ **Foundation (COMPLETE)**
- BootromExploit registry and plugin system
- CPID-based exploit routing (checkm8, usbliter8)
- Post-exploit hook system
- Syringe abstraction layer skeleton
- Cyanide iBoot debugger skeleton
- Anthrax ramdisk builder skeleton
- IpswPatchfinder skeleton
- Complete architecture documentation

⏳ **Next: Fill in the TODOs**

---

## Phase 1: Syringe USB Operations (CRITICAL PATH)

### Task 1.1: Implement DFU Memory Operations
**File:** `src/Syringe.cpp` lines 333-375  
**Status:** Skeleton implemented, needs USB communication  
**Implementation:**

```cpp
bool Syringe::doDFURead(uint64_t addr, uint64_t size, std::vector<uint8_t>& out) {
    // Get DFU device via irecovery
    auto dfu = mDeviceManager->getDFUDevice();
    
    // Use irecovery API:
    // irecovery_send_command(device, "read 0x<addr> 0x<size>");
    // irecovery_get_data(device, out);
    
    out.resize(size);
    // Fill buffer with data from device
    return true;
}
```

**Dependencies:**
- `irecovery_send_command()` - Send USB control message
- `irecovery_get_data()` - Receive USB data
- Device handle from DFUDevice

**Affected Components:**
- CyanidePayload.readMemory() relies on this
- Cyanide REPL "dump" command uses this

### Task 1.2: Implement DFU Memory Write
**File:** `src/Syringe.cpp` lines 377-395  
**Status:** Skeleton implemented, needs USB communication  
**Implementation:**

```cpp
bool Syringe::doDFUWrite(uint64_t addr, const std::vector<uint8_t>& data) {
    auto dfu = mDeviceManager->getDFUDevice();
    
    // Use irecovery API:
    // irecovery_send_buffer(device, data.data(), data.size());
    // irecovery_send_command(device, "write 0x<addr> 0x<size>");
    
    return true;
}
```

**Dependencies:**
- `irecovery_send_buffer()` - Send data to device
- `irecovery_send_command()` - Execute write command

**Affected Components:**
- CyanidePayload.writeMemory() relies on this
- Cyanide REPL "patch" command uses this

### Task 1.3: Recovery Mode Operations
**File:** `src/Syringe.cpp` lines 397-425  
**Status:** Skeleton with error messages  
**Note:** May not be needed initially if DFU operations work

---

## Phase 2: Cyanide REPL Completion

### Task 2.1: Command Parser Enhancement
**File:** `src/CyanidePayload.cpp` lines 243-290  
**Status:** ✅ Complete, all commands recognized

### Task 2.2: Memory Dump Formatting
**File:** `src/CyanidePayload.cpp` lines 115-135  
**Status:** ✅ Complete, uses hex formatting

### Task 2.3: Symbol Resolution
**File:** `src/CyanidePayload.cpp` lines 235-241  
**Status:** Skeleton, needs iBoot symbol table parsing

```cpp
bool CyanidePayload::resolveSymbol(const std::string& symbol, uint64_t& addr) {
    // Parse iBoot's __LINKEDIT segment for symbol table
    // Load from device if needed
    // Return function address
    return false; // TODO
}
```

**Depends on:**
- Parsing iBoot Mach-O format
- Symbol table extraction
- Address resolution

### Task 2.4: Test with Real Device
**Status:** Not started  
**Steps:**
1. Connect device to USB
2. Put in DFU mode
3. Run: `./purplepois0n --iboot-debug iPhone_15.ipsw`
4. Test commands:
   ```
   cyanide> dump 0x80000000 256
   cyanide> mem write 0x8000abcd 00 11 22 33
   cyanide> sym _start
   cyanide> quit
   ```

---

## Phase 3: IpswPatchfinder Implementation

### Task 3.1: ipsw Tool Integration
**File:** `src/IpswPatchfinder.cpp` lines 145-210  
**Status:** Skeleton, needs tool execution

```cpp
bool IpswPatchfinder::runIpswTool(const std::vector<std::string>& args, 
                                   std::string& output) {
    // Build command: "ipsw analyze <cache> ..."
    // Execute via system() or ProcessRunner
    // Capture stdout
    // Parse JSON/text output
    return true;
}
```

**Dependencies:**
- ipsw binary in PATH or bundled
- Process execution and output capture
- JSON/text parsing

### Task 3.2: Kernel Offset Discovery
**File:** `src/IpswPatchfinder.cpp` lines 171-193  
**Status:** Skeleton with hardcoded values

```cpp
bool IpswPatchfinder::discoverOffsets(const ExecutionContext& context, 
                                       KernelOffsets& offsets) {
    // 1. Extract KernelCache from IPSW
    //    ipsw extract <ipsw> KernelCache.decrypted
    
    // 2. Analyze with ipsw
    //    ipsw analyze KernelCache.decrypted
    
    // 3. Parse output for:
    //    - _start / kernel_entry
    //    - boot_args structure
    //    - Function offsets (AMFI, sandbox, etc.)
    
    return true;
}
```

### Task 3.3: Patch Site Matching
**File:** `src/IpswPatchfinder.cpp` lines 207-221  
**Status:** Skeleton  

```cpp
bool IpswPatchfinder::matchPatterns(const std::string& cachePath, 
                                     const std::string& category,
                                     std::vector<PatchSite>& sites) {
    // Load kernelcache binary
    // Search for known patterns:
    //   category="sandbox" → find sandbox patches
    //   category="amfi" → find code signature patches
    //   category="tfp0" → find task_for_pid patches
    
    // For each pattern found:
    //   - Record offset
    //   - Store replacement bytes
    //   - Mark optional or required
    
    sites.push_back({...});
    return true;
}
```

### Task 3.4: Test Pattern Matching
**Status:** Not started  
**Steps:**
1. Download iPhone 15 IPSW
2. Run IpswPatchfinder
3. Verify discovered offsets match known values
4. Compare with ipsw CLI output

---

## Phase 4: Anthrax Ramdisk Building

### Task 4.1: IPSW Extraction
**File:** `src/AnthraxPayload.cpp` lines 157-162  
**Status:** Stub  

```cpp
bool AnthraxPayload::buildBaseRamdisk(const ExecutionContext& context) {
    // Extract from IPSW:
    // - Locate ramdisk image (usually separate or in iBoot)
    // - Decompress (CPIO or IMG format)
    // - Store to /tmp/anthrax_ramdisk_base.cpio
    
    mRamdiskPath = "/tmp/anthrax_ramdisk.cpio";
    return true;
}
```

**Dependencies:**
- IPSW parsing/extraction (ipsw tool)
- CPIO decompression
- Image format detection

### Task 4.2: SSH Server Injection
**File:** `src/AnthraxPayload.cpp` lines 165-169  
**Status:** Stub  

```cpp
bool AnthraxPayload::injectSSHTools() {
    // Add to ramdisk:
    // - Dropbear SSH server binary
    // - SSH keys (/etc/dropbear/authorized_keys)
    // - SSH startup script (/etc/init.d/ssh)
    
    // Rebuild CPIO archive
    return true;
}
```

**What to include:**
- dropbear binary (~400KB)
- OpenSSH (if larger image)
- SSH keys for root user
- Init script to start SSH

### Task 4.3: Patcher Injection
**File:** `src/AnthraxPayload.cpp` lines 171-175  
**Status:** Stub  

```cpp
bool AnthraxPayload::injectPatchers() {
    // Add to ramdisk:
    // - kpf-purple binary (kernel patcher)
    // - Patch definitions (AMFI, sandbox, etc.)
    // - Patcher script (/usr/local/bin/patch-kernel.sh)
    
    return true;
}
```

**What to include:**
- kpf-purple kernel patcher
- Patch profile JSON files
- Helper scripts
- Utilities (mount, launchctl, etc.)

### Task 4.4: Ramdisk Compression
**File:** `src/AnthraxPayload.cpp` lines 177-181  
**Status:** Stub  

```cpp
bool AnthraxPayload::compressRamdisk() {
    // Recompress CPIO to format boot expects:
    // - iOS 4-8: CPIO.GZ
    // - iOS 9+: IMG3 with LZSS compression
    
    // Result: Ready to pass to iBoot
    return true;
}
```

---

## Phase 5: iBoot Patching & Loading

### Task 5.1: Extract iBoot from IPSW
**File:** Needs new file `src/IBootExtractor.cpp`  
**Status:** Not started  

```cpp
bool extractIBootFromIPSW(const std::string& ipswPath,
                          std::vector<uint8_t>& iboot) {
    // 1. Open IPSW (it's a ZIP file)
    // 2. Locate iBoot binary (usually "iBoot.<version>.RELEASE")
    // 3. Decrypt using keys from Firmware/all_flash/
    // 4. Return decrypted iBoot
    
    return true;
}
```

**Dependencies:**
- ZIP file handling
- iBoot decryption (use ipsw tool)
- Image3 format parsing

### Task 5.2: Patch iBoot Binary
**File:** Needs new file `src/IBootPatcher.cpp`  
**Status:** Not started  

```cpp
bool patchIBoot(std::vector<uint8_t>& iboot,
                 const KernelOffsets& offsets) {
    // Patch iBoot to:
    // 1. Disable signature verification
    //    - Find "cert check" function, NOP it out
    // 2. Change ramdisk path
    //    - Modify boot argument handling
    // 3. Disable security checks
    //    - Codesign verification
    //    - Trust cache validation
    
    return true;
}
```

### Task 5.3: Load Patched iBoot to Device
**File:** `src/AnthraxPayload.cpp` (injectIntoBootChain)  
**Status:** Stub  

```cpp
bool AnthraxPayload::injectIntoBootChain(DeviceManager& manager,
                                          const ExecutionContext& context) {
    // 1. Extract iBoot from IPSW
    // 2. Patch iBoot (disable sig check, modify boot args)
    // 3. Load patched iBoot to device via Syringe
    
    auto syringe = std::make_shared<Syringe>();
    syringe->detectDevice(manager);
    syringe->sendFile("/tmp/iBoot.patched", 0x80000000);
    
    return true;
}
```

---

## Phase 6: End-to-End Testing

### Test 1: Device Detection & Routing
**Expected:**
```bash
./purplepois0n --list
# Output: Device in DFU (CPID 0x8974)
```

### Test 2: Cyanide REPL
**Expected:**
```bash
./purplepois0n --iboot-debug iPhone_15.ipsw
cyanide> dump 0x80000000 256
# Output: hex dump of iBoot memory
```

### Test 3: Anthrax Ramdisk Boot
**Expected:**
```bash
./purplepois0n --anthrax-ramdisk iPhone_15.ipsw --boot
# Device boots with SSH running
$ ssh root@<device-ip>
# Remote shell works
```

### Test 4: Live Kernel Patching
**Expected:**
```bash
device# kpf-patcher --sandbox
device# launchctl start app
# App runs in jailbroken environment
```

---

## Dependencies & Libraries

### Required (Already Available)
- ✅ libimobiledevice-1.0 (device communication)
- ✅ libirecovery-1.0 (DFU/recovery mode)
- ✅ libusb-1.0 (low-level USB)
- ✅ libplist-2.0 (XML parsing)

### Needed
- ⏳ ipsw tool (for patchfinding)
- ⏳ CPIO handling (for ramdisk)
- ⏳ ZIP handling (for IPSW)
- ⏳ Image3/IMG4 parsing (for iBoot/kernel)

### Optional
- dropbear or OpenSSH (for ramdisk SSH)
- kpf-purple (kernel patcher framework)

---

## Estimated Timeline

| Phase | Task | Est. Hours | Priority |
|-------|------|-----------|----------|
| 1 | Syringe USB ops | 8-12 | 🔴 CRITICAL |
| 2 | Cyanide REPL finish | 4-6 | 🔴 CRITICAL |
| 3 | IpswPatchfinder | 8-10 | 🔴 CRITICAL |
| 4 | Anthrax ramdisk | 12-16 | 🟠 HIGH |
| 5 | iBoot patching | 10-14 | 🟠 HIGH |
| 6 | Testing & fixes | 8-12 | 🟠 HIGH |
| **TOTAL** | | **50-70 hours** | |

---

## Quality Checklist

- [ ] All TODO comments replaced with real implementations
- [ ] Error handling for all external tool invocations
- [ ] Unit tests for IpswPatchfinder, Syringe, CyanidePayload
- [ ] Real device testing (iPhone 6S, iPhone 12, iPhone 15)
- [ ] Documentation for each new function
- [ ] Examples in ARCHITECTURE.md and other docs
- [ ] Security review (no hardcoded passwords, safe USB handling)
- [ ] CI/CD integration (automated builds)

---

## Known Limitations & Workarounds

### Limitation: ipsw Tool Availability
**Workaround:** Bundled precompiled ipsw binary or download from GitHub releases

### Limitation: Device-Specific Offsets
**Workaround:** Use IpswPatchfinder to discover automatically (already designed for this)

### Limitation: SSH Key Distribution
**Workaround:** Generate keys during ramdisk build, embed in image

### Limitation: Ramdisk Size
**Workaround:** Minimize with --strip, use compressed filesystems

### Limitation: Slow USB Operations
**Workaround:** Batch operations, use larger transfers

---

## Success Criteria

✅ **Minimum Viable Product:**
1. Device detection works
2. Bootrom exploit routes correctly
3. Cyanide REPL reads iBoot memory
4. IpswPatchfinder discovers offsets for iPhone 6S and iPhone 15

✅ **Full Implementation:**
1. All of above, plus:
2. Anthrax ramdisk builds successfully
3. Ramdisk boots on real device
4. SSH access works
5. Live kernel patches apply

✅ **Production Ready:**
1. All of above, plus:
2. Comprehensive testing on 5+ device types
3. Error recovery and retry logic
4. User-friendly documentation
5. Educational walkthroughs

