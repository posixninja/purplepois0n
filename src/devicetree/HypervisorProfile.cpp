/*
 * HypervisorProfile.cpp
 */

#include "devicetree/HypervisorProfile.h"
#include "devicetree/DeviceTreeCatalog.h"
#include "primitives/Gen6Types.h"
#include "primitives/PrimitiveTypes.h"
#include "EnvUtil.h"
#include "Logger.h"

#include <fstream>
#include <sstream>

namespace PP {
namespace devicetree {

namespace {

static const char* kHypervisorNeedles[] = {
    "SPTM",
    "com.apple.private.hypervisor",
    "hv_vcpu",
    "hv_vm_t",
    "stage-2",
    "sptm_retype",
    "PageTableMonitor",
};

bool scanKernelcacheForHypervisor(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) {
        return false;
    }
    std::vector<char> blob((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (blob.size() < 16) {
        return false;
    }
    const std::string haystack(blob.begin(), blob.end());
    for (size_t i = 0; i < sizeof(kHypervisorNeedles) / sizeof(kHypervisorNeedles[0]); ++i) {
        if (haystack.find(kHypervisorNeedles[i]) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void appendStrategy(std::vector<PageControlStrategy>* out, PageControlStrategy strategy) {
    if (out == nullptr) {
        return;
    }
    for (size_t i = 0; i < out->size(); ++i) {
        if ((*out)[i] == strategy) {
            return;
        }
    }
    out->push_back(strategy);
}

bool socLikelyPageMonitor(const DeviceTreeSummary& summary) {
    if (summary.pageMonitorPresent || summary.hasVirtualization) {
        return true;
    }
    if (summary.socGeneration.compare(0, 3, "t81") == 0 ||
        summary.socGeneration.compare(0, 3, "t82") == 0) {
        return true;
    }
    if (summary.socName.find("A15") != std::string::npos ||
        summary.socName.find("A16") != std::string::npos ||
        summary.socName.find("A17") != std::string::npos ||
        summary.socName.find("A18") != std::string::npos) {
        return true;
    }
    return false;
}

} /* anonymous */

const char* pageAuthorityToString(PageAuthority authority) {
    switch (authority) {
        case PageAuthority::XnuDirect:
            return "XNU-direct";
        case PageAuthority::PplCoprocessor:
            return "PPL-coprocessor";
        case PageAuthority::SptmHypervisor:
            return "SPTM-hypervisor";
    }
    return "unknown";
}

const char* pageControlStrategyToString(PageControlStrategy strategy) {
    switch (strategy) {
        case PageControlStrategy::PreBootKpf:
            return "pre-boot-KPF";
        case PageControlStrategy::BypassPplMmio:
            return "PPL-MMIO-bypass";
        case PageControlStrategy::SptmFrameRetype:
            return "SPTM-frame-retype";
        case PageControlStrategy::Stage2PageTables:
            return "hypervisor-stage2-PT";
        case PageControlStrategy::HypervisorEntitlement:
            return "private-hypervisor-entitlement";
        case PageControlStrategy::XpfPhysmapOffsets:
            return "XPF-physmap-offsets";
    }
    return "unknown";
}

HypervisorProfile buildHypervisorProfile(const DeviceTreeSummary& summary, uint32_t cpid,
                                         const std::string& iosVersion,
                                         const std::string& kernelcachePath) {
    HypervisorProfile profile;
    profile.hasVirtualization = summary.hasVirtualization;
    profile.pageMonitorPresent = summary.pageMonitorPresent;
    profile.kernelHypervisorStrings = scanKernelcacheForHypervisor(kernelcachePath);

    const bool ios17Plus =
        !iosVersion.empty() && primitives::iosVersionInRange(iosVersion, "17.0", "99.0");
    const bool ios15Plus =
        !iosVersion.empty() && primitives::iosVersionInRange(iosVersion, "15.0", "99.0");
    const bool a15PlusSilicon = socLikelyPageMonitor(summary);

    if (profile.hasVirtualization || profile.pageMonitorPresent || ios17Plus ||
        profile.kernelHypervisorStrings || (a15PlusSilicon && !iosVersion.empty() && ios15Plus)) {
        profile.authority = PageAuthority::SptmHypervisor;
        profile.pageMonitorPresent = true;
    } else if (a15PlusSilicon && iosVersion.empty()) {
        profile.authority = PageAuthority::SptmHypervisor;
        profile.pageMonitorPresent = true;
        profile.summary =
            "A15+ silicon — SPTM/hypervisor likely on iOS 17+; pass iOS version to confirm";
    } else if (ios15Plus) {
        profile.authority = PageAuthority::PplCoprocessor;
    } else {
        profile.authority = PageAuthority::XnuDirect;
    }

    switch (profile.authority) {
        case PageAuthority::SptmHypervisor:
            appendStrategy(&profile.strategies, PageControlStrategy::XpfPhysmapOffsets);
            appendStrategy(&profile.strategies, PageControlStrategy::SptmFrameRetype);
            appendStrategy(&profile.strategies, PageControlStrategy::Stage2PageTables);
            appendStrategy(&profile.strategies, PageControlStrategy::HypervisorEntitlement);
            profile.summary =
                "SPTM/hypervisor owns frame retyping and stage-2 mappings; XNU cannot patch "
                "pages directly — control the page monitor or hypervisor API.";
            break;
        case PageAuthority::PplCoprocessor:
            appendStrategy(&profile.strategies, PageControlStrategy::BypassPplMmio);
            appendStrategy(&profile.strategies, PageControlStrategy::XpfPhysmapOffsets);
            appendStrategy(&profile.strategies, PageControlStrategy::PreBootKpf);
            profile.summary =
                "PPL coprocessor mediates kernel page tables; dmaFail-class MMIO bypass or "
                "pre-boot KPF required before live patches stick.";
            break;
        case PageAuthority::XnuDirect:
            appendStrategy(&profile.strategies, PageControlStrategy::PreBootKpf);
            appendStrategy(&profile.strategies, PageControlStrategy::XpfPhysmapOffsets);
            profile.summary =
                "Classic XNU page tables — pre-boot KPF or direct physrw suffices for hooks.";
            break;
    }

    if (cpid != 0) {
        std::ostringstream oss;
        oss << profile.summary << " (CPID 0x" << std::hex << cpid << std::dec << ")";
        profile.summary = oss.str();
    }
    if (!summary.socName.empty()) {
        profile.summary += " soc=" + summary.socName;
    }
    return profile;
}

HypervisorProfile buildHypervisorProfileFromContext(const primitives::ExecutionContext& context) {
    DeviceTreeSummary summary;
    if (!loadGlobalCatalogFromEnv()) {
        /* optional */
    }
    const DeviceTreeCatalog* catalog = globalCatalog();
    if (catalog != nullptr && catalog->success) {
        summary = catalog->summary;
    } else if (!context.ipswPath.empty()) {
        const DeviceTreeCatalog built = buildCatalogFromPath(context.ipswPath, false);
        if (built.success) {
            summary = built.summary;
        }
    }

    std::string kernelPath = context.kernelcachePath;
    if (kernelPath.empty()) {
        kernelPath = envOrEmpty("PURPLEPOIS0N_KERNELCACHE");
    }

    return buildHypervisorProfile(summary, context.cpid, context.iosVersion, kernelPath);
}

bool pageMonitorRequired(const HypervisorProfile& profile) {
    return profile.authority == PageAuthority::SptmHypervisor || profile.pageMonitorPresent;
}

void logHypervisorControlPlan(const HypervisorProfile& profile) {
    Logger::info(std::string("  [PageMonitor] authority: ") +
                 pageAuthorityToString(profile.authority));
    if (profile.hasVirtualization) {
        Logger::info("  [PageMonitor] DT has-virtualization=1 (embedded hypervisor enabled)");
    }
    if (profile.kernelHypervisorStrings) {
        Logger::info("  [PageMonitor] kernelcache contains hypervisor/SPTM symbols");
    }
    Logger::info("  [PageMonitor] " + profile.summary);
    for (size_t i = 0; i < profile.strategies.size(); ++i) {
        Logger::info(std::string("  [PageMonitor]   strategy: ") +
                     pageControlStrategyToString(profile.strategies[i]));
    }
}

} /* namespace devicetree */
} /* namespace PP */
