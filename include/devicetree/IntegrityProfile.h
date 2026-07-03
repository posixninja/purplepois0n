/*
 * IntegrityProfile.h
 *
 * PAC (code/data pointers) and data-integrity (MIE) bypass planning.
 */

#ifndef DEVICETREE_INTEGRITY_PROFILE_H_
#define DEVICETREE_INTEGRITY_PROFILE_H_

#include "devicetree/MmioTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PP {
namespace primitives {
struct ExecutionContext;
} /* namespace primitives */

namespace devicetree {

enum class IntegrityLayer {
    None,
    PacCode,
    PacData,
    MieData
};

enum class IntegrityBypassStrategy {
    BadRecoveryDelegate,
    PacSigningGadget,
    PtrauthStripPreBoot,
    KernelPacDisablePatch,
    MieResearchDelegate,
    AmfiBypassHookPath
};

struct IntegrityProfile {
    bool arm64e = false;
    bool pacCodeRequired = false;
    bool pacDataRequired = false;
    bool mieDataRequired = false;
    bool kernelPacStrings = false;
    bool kernelMieStrings = false;
    std::vector<IntegrityLayer> layers;
    std::vector<IntegrityBypassStrategy> strategies;
    /** Primary Dopamine / env delegate for one-shot bypass (e.g. badRecovery). */
    std::string recommendedModule;
    std::string recommendedEnvKey;
    std::string summary;
};

const char* integrityLayerToString(IntegrityLayer layer);
const char* integrityBypassStrategyToString(IntegrityBypassStrategy strategy);

IntegrityProfile buildIntegrityProfile(const DeviceTreeSummary& summary,
                                       const std::string& productType,
                                       const std::string& iosVersion,
                                       bool arm64eHint,
                                       const std::string& kernelcachePath = "");

IntegrityProfile buildIntegrityProfileFromContext(const primitives::ExecutionContext& context);

bool pacBypassRequired(const IntegrityProfile& profile);
bool dataIntegrityBypassRequired(const IntegrityProfile& profile);

/** Module/env to load for automatic PAC+data bypass on this platform. */
std::string recommendedIntegrityBypassEnv(const IntegrityProfile& profile);

void logIntegrityBypassPlan(const IntegrityProfile& profile);

} /* namespace devicetree */
} /* namespace PP */

#endif /* DEVICETREE_INTEGRITY_PROFILE_H_ */
