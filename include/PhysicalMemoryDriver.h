/*
 * PhysicalMemoryDriver.h
 *
 * Kernel driver for direct physical memory access.
 * Loaded after bootrom exploit to provide userland with:
 * - Direct read/write to physical memory
 * - Page table manipulation
 * - Physical page allocation tracking
 * - Freed page detection
 *
 * Bridges bootrom jailbreak → kernel control → userland physical memory access
 */

#ifndef PHYSICAL_MEMORY_DRIVER_H_
#define PHYSICAL_MEMORY_DRIVER_H_

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace PP {

class Syringe;
class DeviceManager;

/**
 * @enum PhysMemDriverIOCTL
 * @brief IOCTLs for physical memory driver communication.
 */
enum PhysMemDriverIOCTL : uint32_t {
    READ_PHYS_MEM = 0x80000001U,    // Read physical memory
    WRITE_PHYS_MEM = 0x80000002U,   // Write physical memory
    MAP_PHYS = 0x80000003U,         // Map physical page to userland
    UNMAP_PHYS = 0x80000004U,       // Unmap physical page
    GET_PAGE_TABLE = 0x80000005U,   // Get page table entries
    SET_PAGE_TABLE = 0x80000006U,   // Modify page table entries
    DETECT_FREED_PAGES = 0x80000007U,  // Find freed pages
    ALLOCATE_CONTROLLED = 0x80000008U, // Allocate at specific address
    QUERY_PHYS_LAYOUT = 0x80000009U,   // Query memory layout
    PATCH_KERNEL = 0x8000000AU,     // Apply kernel patches
    ENABLE_DEBUGGER = 0x8000000BU   // Enable kernel debugger
};

/**
 * @struct PhysMemRequest
 * @brief Request structure for physical memory operations.
 */
struct PhysMemRequest {
    PhysMemDriverIOCTL operation;
    uint64_t physicalAddr;
    uint64_t size;
    uint64_t virtualAddr;      // For mapping operations
    std::vector<uint8_t> data; // For read/write operations
    uint32_t flags;
};

/**
 * @struct PhysMemResponse
 * @brief Response from physical memory driver operations.
 */
struct PhysMemResponse {
    bool success;
    uint64_t resultAddr;       // For mapping/allocation
    std::vector<uint8_t> data; // For read operations
    std::string error;
    uint32_t resultCode;
};

/**
 * @class PhysicalMemoryDriver
 * @brief Kernel-level driver for physical memory access.
 *        Loaded after bootrom exploit to grant full physical memory control.
 */
class PhysicalMemoryDriver {
public:
    PhysicalMemoryDriver();
    ~PhysicalMemoryDriver();

    // Driver lifecycle
    bool loadDriver(std::shared_ptr<Syringe> syringe);
    bool unloadDriver();
    bool isLoaded() const { return mLoaded; }

    // Physical memory access via kernel driver
    bool readPhysicalMemory(uint64_t physAddr, uint64_t size, std::vector<uint8_t>& out);
    bool writePhysicalMemory(uint64_t physAddr, const std::vector<uint8_t>& data);

    // Page table manipulation
    bool mapPhysicalPage(uint64_t physAddr, uint64_t& virtualAddr);
    bool unmapPhysicalPage(uint64_t virtualAddr);
    bool getPageTableEntry(uint64_t virtualAddr, uint64_t& pte);
    bool setPageTableEntry(uint64_t virtualAddr, uint64_t pte);

    // Memory detection and tracking
    bool detectFreedPages(std::vector<uint64_t>& freedPages);
    bool queryMemoryLayout(std::map<std::string, uint64_t>& layout);
    bool trackPageAllocations();

    // Controlled allocation
    bool allocateAtAddress(uint64_t addr, uint64_t size);
    bool deallocateAtAddress(uint64_t addr);

    // Kernel patching via driver
    bool applyKernelPatch(uint64_t addr, const std::vector<uint8_t>& patch);
    bool enableKernelDebugger();

    // Verification
    bool verifyDriverAccess();

private:
    bool mLoaded;
    std::shared_ptr<Syringe> mSyringe;
    uint64_t mDriverBase;
    int mDriverFD;  // File descriptor for driver device

    // Internal helpers
    bool locateDriver();
    bool openDriverDevice();
    bool sendIOCTL(const PhysMemRequest& req, PhysMemResponse& resp);
    bool buildDriverPayload();
    bool injectIntoKernel();
};

/**
 * @class BootromPhysicalMemoryBridge
 * @brief Orchestrates bootrom exploit → kernel driver → physical memory access
 */
class BootromPhysicalMemoryBridge {
public:
    BootromPhysicalMemoryBridge();
    ~BootromPhysicalMemoryBridge();

    // Full exploit chain
    bool executeFullChain(std::shared_ptr<Syringe> syringe,
                         std::shared_ptr<DeviceManager> manager);

    // Access physical memory after jailbreak
    bool readPhysical(uint64_t addr, uint64_t size, std::vector<uint8_t>& out);
    bool writePhysical(uint64_t addr, const std::vector<uint8_t>& data);

    // High-level operations
    bool exploitFreedObject(uint64_t physAddr, uint64_t size);
    bool gainKernelControl();
    bool escapeUserland();

private:
    std::shared_ptr<PhysicalMemoryDriver> mDriver;
    bool mJailbroken;
    bool mKernelControlGranted;

    // Chain components
    bool runBootromExploit();
    bool loadPhysicalMemoryDriver();
    bool verifyFullChain();
};

} /* namespace PP */

#endif /* PHYSICAL_MEMORY_DRIVER_H_ */
