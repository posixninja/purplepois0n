/*
 * IpswPatchfinder.h
 *
 * Device-agnostic patchfinding via ipsw tool.
 * Discovers kernel offsets, boot functions, and patch sites across all devices
 * (A5-A13+, S4-S5) without bundling device-specific offsets.
 */

#ifndef IPSW_PATCHFINDER_H_
#define IPSW_PATCHFINDER_H_

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "primitives/PrimitiveTypes.h"

namespace PP {

using primitives::ExecutionContext;

class AppleCvsDatabase;

/**
 * @struct PatchSite
 * @brief Describes a location in kernel where a patch should be applied.
 */
struct PatchSite {
    std::string name;
    std::string description;
    uint64_t offset;
    std::vector<uint8_t> pattern;
    std::vector<uint8_t> replacement;
    bool optional;
};

/**
 * @struct KernelOffsets
 * @brief Device-specific offsets discovered via ipsw.
 */
struct KernelOffsets {
    uint64_t kernelEntry;
    uint64_t kernelStart;
    uint64_t bootArgsAddr;
    uint64_t bootArgsSize;
    std::string boardConfig;
    uint32_t cpid;
    std::map<std::string, uint64_t> functionOffsets;
    std::vector<PatchSite> patchSites;
};

/**
 * @class IpswPatchfinder
 * @brief Automatic patchfinding using ipsw tool.
 *        Requires ipsw binary in PATH or bundled.
 *        Works uniformly across all device generations.
 */
class IpswPatchfinder {
public:
    IpswPatchfinder();
    ~IpswPatchfinder();

    // Patchfinding entry point
    bool discoverOffsets(const ExecutionContext& context, KernelOffsets& offsets);

    // Find specific kernel functions/structures
    bool findKernelEntry(const ExecutionContext& context, uint64_t& addr);
    bool findBootArgs(const ExecutionContext& context, uint64_t& addr, uint64_t& size);
    bool findFunction(const ExecutionContext& context, const std::string& funcName,
                      uint64_t& addr);

    // Find patch sites (e.g., code integrity, sandbox, etc.)
    bool findPatchSites(const ExecutionContext& context, const std::string& category,
                        std::vector<PatchSite>& sites);

    // Check if ipsw tool is available
    static bool isAvailable();

    // Get ipsw tool path
    static std::string getIpswPath();

private:
    std::string mIpswPath;
    bool mInitialized;
    std::shared_ptr<AppleCvsDatabase> mCvsDatabase;

    // Internal patchfinding operations
    bool extractKernelCache(const ExecutionContext& context, std::string& cachePath);
    bool parseIpswOutput(const std::string& output, KernelOffsets& offsets);
    bool runIpswTool(const std::vector<std::string>& args, std::string& output);

    // Device-agnostic pattern matching
    bool matchPatterns(const std::string& cachePath, const std::string& category,
                       std::vector<PatchSite>& sites);

    // CVS database integration
    bool enrichPatchesWithHistory(std::vector<PatchSite>& patches);
};

} /* namespace PP */

#endif /* IPSW_PATCHFINDER_H_ */
