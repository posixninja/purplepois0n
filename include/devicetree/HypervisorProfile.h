/*
 * HypervisorProfile.h
 *
 * SPTM / hypervisor page-mapping authority and control strategy (offline + DT).
 */

#ifndef DEVICETREE_HYPERVISOR_PROFILE_H_
#define DEVICETREE_HYPERVISOR_PROFILE_H_

#include "devicetree/MmioTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PP {
namespace primitives {
struct ExecutionContext;
} /* namespace primitives */

namespace devicetree {

/** Who owns physical page table / frame retyping on this platform. */
enum class PageAuthority {
    XnuDirect,
    PplCoprocessor,
    SptmHypervisor
};

/** How to regain control over page mappings once authority is externalized. */
enum class PageControlStrategy {
    PreBootKpf,
    BypassPplMmio,
    SptmFrameRetype,
    Stage2PageTables,
    HypervisorEntitlement,
    XpfPhysmapOffsets
};

struct HypervisorProfile {
    PageAuthority authority = PageAuthority::XnuDirect;
    bool hasVirtualization = false;
    bool pageMonitorPresent = false;
    bool kernelHypervisorStrings = false;
    std::vector<PageControlStrategy> strategies;
    std::string summary;
};

const char* pageAuthorityToString(PageAuthority authority);
const char* pageControlStrategyToString(PageControlStrategy strategy);

HypervisorProfile buildHypervisorProfile(const DeviceTreeSummary& summary, uint32_t cpid,
                                         const std::string& iosVersion,
                                         const std::string& kernelcachePath = "");

HypervisorProfile buildHypervisorProfileFromContext(const primitives::ExecutionContext& context);

bool pageMonitorRequired(const HypervisorProfile& profile);

void logHypervisorControlPlan(const HypervisorProfile& profile);

} /* namespace devicetree */
} /* namespace PP */

#endif /* DEVICETREE_HYPERVISOR_PROFILE_H_ */
