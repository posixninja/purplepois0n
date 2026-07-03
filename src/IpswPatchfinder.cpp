/*
 * IpswPatchfinder.cpp
 */

#include "IpswPatchfinder.h"
#include "AppleCvsDatabase.h"
#include "Logger.h"
#include "ToolRunner.h"
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <algorithm>

namespace PP {

IpswPatchfinder::IpswPatchfinder() : mInitialized(false) {
    // Initialize Apple CVS database for patch verification
    mCvsDatabase = std::make_shared<AppleCvsDatabase>();
    mCvsDatabase->initialize();

    if (isAvailable()) {
        mIpswPath = getIpswPath();
        mInitialized = true;
    }
}

IpswPatchfinder::~IpswPatchfinder() {}

bool IpswPatchfinder::discoverOffsets(const ExecutionContext& context, KernelOffsets& offsets) {
    if (!mInitialized) {
        Logger::error("IpswPatchfinder: ipsw tool not available");
        return false;
    }

    Logger::info("IpswPatchfinder: Discovering device offsets...");

    // Stage 1: Extract kernel cache from IPSW
    std::string cachePath;
    if (!extractKernelCache(context, cachePath)) {
        Logger::error("IpswPatchfinder: Failed to extract kernel cache");
        return false;
    }

    // Stage 2: Analyze kernel cache for offsets
    std::vector<std::string> args = {
        mIpswPath, "analyze", cachePath,
        "--find-kernel-entry", "--find-boot-args", "--parse-macho"
    };

    std::string output;
    if (!runIpswTool(args, output)) {
        Logger::error("IpswPatchfinder: ipsw tool failed");
        return false;
    }

    // Stage 3: Parse ipsw output and populate offsets
    if (!parseIpswOutput(output, offsets)) {
        Logger::error("IpswPatchfinder: Failed to parse ipsw output");
        return false;
    }

    // Stage 4: Find patch sites
    if (!findPatchSites(context, "common", offsets.patchSites)) {
        Logger::warn("IpswPatchfinder: No patch sites found");
    }

    Logger::info("IpswPatchfinder: Offset discovery complete");
    return true;
}

bool IpswPatchfinder::findKernelEntry(const ExecutionContext& context, uint64_t& addr) {
    Logger::debug("IpswPatchfinder: Finding kernel entry point...");

    KernelOffsets offsets;
    if (!discoverOffsets(context, offsets)) {
        return false;
    }

    addr = offsets.kernelEntry;
    return addr != 0;
}

bool IpswPatchfinder::findBootArgs(const ExecutionContext& context, uint64_t& addr,
                                    uint64_t& size) {
    Logger::debug("IpswPatchfinder: Finding boot arguments...");

    KernelOffsets offsets;
    if (!discoverOffsets(context, offsets)) {
        return false;
    }

    addr = offsets.bootArgsAddr;
    size = offsets.bootArgsSize;
    return addr != 0;
}

bool IpswPatchfinder::findFunction(const ExecutionContext& context, const std::string& funcName,
                                    uint64_t& addr) {
    Logger::debug("IpswPatchfinder: Finding function: " + funcName);

    KernelOffsets offsets;
    if (!discoverOffsets(context, offsets)) {
        return false;
    }

    auto it = offsets.functionOffsets.find(funcName);
    if (it == offsets.functionOffsets.end()) {
        Logger::warn("IpswPatchfinder: Function not found: " + funcName);
        return false;
    }

    addr = it->second;
    return true;
}

bool IpswPatchfinder::findPatchSites(const ExecutionContext& context, const std::string& category,
                                      std::vector<PatchSite>& sites) {
    Logger::debug("IpswPatchfinder: Finding patch sites for category: " + category);

    if (!mInitialized) {
        return false;
    }

    std::string cachePath;
    if (!extractKernelCache(context, cachePath)) {
        return false;
    }

    return matchPatterns(cachePath, category, sites);
}

bool IpswPatchfinder::isAvailable() {
    // Try to find ipsw in PATH or common locations
    const char* paths[] = {
        "ipsw",
        "/usr/local/bin/ipsw",
        "/opt/homebrew/bin/ipsw",
        "/usr/bin/ipsw"
    };

    for (const auto& path : paths) {
        std::string cmd = std::string("which ") + path;
        if (system(cmd.c_str()) == 0) {
            return true;
        }
    }

    return false;
}

std::string IpswPatchfinder::getIpswPath() {
    // Return first available ipsw path
    const char* paths[] = {
        "ipsw",
        "/usr/local/bin/ipsw",
        "/opt/homebrew/bin/ipsw",
        "/usr/bin/ipsw"
    };

    for (const auto& path : paths) {
        std::string cmd = std::string("which ") + path;
        if (system(cmd.c_str()) == 0) {
            return path;
        }
    }

    return "ipsw";
}

bool IpswPatchfinder::extractKernelCache(const ExecutionContext& context, std::string& cachePath) {
    Logger::debug("IpswPatchfinder: Extracting kernel cache from IPSW...");

    if (context.ipswPath.empty()) {
        Logger::error("IpswPatchfinder: No IPSW path provided");
        return false;
    }

    // Use ipsw extract tool
    std::vector<std::string> args = {
        mIpswPath, "extract", context.ipswPath, "--kernel-cache"
    };

    std::string output;
    if (!runIpswTool(args, output)) {
        Logger::error("IpswPatchfinder: Failed to extract kernel cache");
        return false;
    }

    // ipsw extract outputs to /tmp by default
    cachePath = "/tmp/kernelcache.decrypted";
    Logger::debug("IpswPatchfinder: Kernel cache extracted to: " + cachePath);
    return true;
}

bool IpswPatchfinder::parseIpswOutput(const std::string& output, KernelOffsets& offsets) {
    Logger::debug("IpswPatchfinder: Parsing ipsw analyze output...");

    if (output.empty()) {
        Logger::warn("IpswPatchfinder: Empty ipsw output, using defaults");
        offsets.kernelEntry = 0x800007f8;
        offsets.kernelStart = 0x80000000;
        offsets.bootArgsAddr = 0x80000000 + 0x7f8;
        offsets.bootArgsSize = 0x1000;
        return true;
    }

    // Parse ipsw analyze output for offsets
    // Example output contains:
    // "kernel_entry: 0x800007f8"
    // "boot_args: 0x800007f8"
    // "board_config: N104AP"

    // Basic parsing: search for key patterns
    size_t pos = output.find("kernel_entry");
    if (pos != std::string::npos) {
        // Extract hex value after "0x"
        size_t hexPos = output.find("0x", pos);
        if (hexPos != std::string::npos) {
            std::string hexStr = output.substr(hexPos, 10);
            offsets.kernelEntry = std::stoull(hexStr, nullptr, 16);
        }
    }

    // Default values if not found
    if (offsets.kernelEntry == 0) {
        offsets.kernelEntry = 0x800007f8;
    }
    offsets.kernelStart = 0x80000000;
    offsets.bootArgsAddr = 0x80000000 + 0x7f8;
    offsets.bootArgsSize = 0x1000;

    std::ostringstream oss;
    oss << "IpswPatchfinder: Parsed offsets: kernel_entry=0x" << std::hex << offsets.kernelEntry;
    Logger::debug(oss.str());

    return true;
}

bool IpswPatchfinder::runIpswTool(const std::vector<std::string>& args, std::string& output) {
    Logger::debug("IpswPatchfinder: Running ipsw tool...");

    // Build command string
    std::ostringstream cmd;
    for (const auto& arg : args) {
        cmd << arg << " ";
    }

    // Execute and capture output
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        Logger::error("IpswPatchfinder: Failed to execute ipsw tool");
        return false;
    }

    // Read output
    char buffer[256];
    output.clear();
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int status = pclose(pipe);
    if (status != 0) {
        std::ostringstream oss;
        oss << "IpswPatchfinder: ipsw tool returned status " << status;
        Logger::warn(oss.str());
        return false;
    }

    return true;
}

bool IpswPatchfinder::matchPatterns(const std::string& cachePath, const std::string& category,
                                     std::vector<PatchSite>& sites) {
    Logger::debug("IpswPatchfinder: Matching patterns for category: " + category);

    if (!std::ifstream(cachePath)) {
        Logger::error("IpswPatchfinder: Kernel cache not found: " + cachePath);
        return false;
    }

    // Read kernel cache binary
    std::ifstream file(cachePath, std::ios::binary);
    if (!file) {
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> cache(fileSize);
    file.read((char*)cache.data(), fileSize);
    file.close();

    // Define patch patterns for each category
    struct PatternMatch {
        std::string name;
        std::string desc;
        std::vector<uint8_t> pattern;
        std::vector<uint8_t> replacement;
    };

    std::vector<PatternMatch> patterns;

    if (category == "sandbox" || category == "common") {
        // Sandbox bypass pattern (varies by iOS version)
        patterns.push_back({
            "sandbox_bypass",
            "Disable sandbox restrictions",
            {0x00, 0xb1, 0x00, 0x24},  // Pattern
            {0x00, 0xb1, 0x01, 0x24}   // Replacement
        });
    }

    if (category == "amfi" || category == "common") {
        // AMFI code signature check
        patterns.push_back({
            "amfi_disable",
            "Disable code signature verification",
            {0x01, 0x20, 0x40, 0x42},
            {0x00, 0x20, 0x00, 0x20}
        });
    }

    // Search cache for patterns
    for (const auto& pat : patterns) {
        for (size_t i = 0; i <= cache.size() - pat.pattern.size(); i++) {
            if (std::equal(pat.pattern.begin(), pat.pattern.end(),
                          cache.begin() + i)) {
                sites.push_back({
                    pat.name,
                    pat.desc,
                    i,  // offset in cache
                    pat.pattern,
                    pat.replacement,
                    false  // required
                });

                std::ostringstream oss;
                oss << "IpswPatchfinder: Found patch site '" << pat.name
                    << "' at offset 0x" << std::hex << i;
                Logger::debug(oss.str());
            }
        }
    }

    return !sites.empty() || category != "common";
}

bool IpswPatchfinder::enrichPatchesWithHistory(std::vector<PatchSite>& patches) {
    if (!mCvsDatabase) {
        return false;
    }

    Logger::debug("IpswPatchfinder: Enriching patches with Apple CVS history...");

    for (auto& patch : patches) {
        PatchHistory history;
        if (mCvsDatabase->findPatchHistory(patch.name, history)) {
            std::ostringstream oss;
            oss << patch.description << " [" << history.cveId << ", "
                << history.appleSecurityBulletin << "]";
            patch.description = oss.str();

            Logger::debug("IpswPatchfinder: Enriched patch: " + patch.name);
        }
    }

    return true;
}

} /* namespace PP */
