/*
 * IntegrityProfile.cpp
 */

#include "devicetree/IntegrityProfile.h"
#include "devicetree/DeviceTreeCatalog.h"
#include "primitives/Gen6Types.h"
#include "EnvUtil.h"
#include "Logger.h"

#include <fstream>

namespace PP {
namespace devicetree {

namespace {

static const char* kPacNeedles[] = {
    "ptrauth",
    "pacib",
    "pacda",
    "pacga",
    "PACIB",
    "ptrauth_sign",
    "com.apple.private.pac",
};

static const char* kMieNeedles[] = {
    "MemoryIntegrity",
    "memory_integrity",
    "mie_",
    "MIE",
    "GPI",
    "guard_metadata",
};

bool scanKernelcache(const std::string& path, const char* const* needles, size_t count) {
    if (path.empty()) {
        return false;
    }
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) {
        return false;
    }
    const std::string haystack((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    for (size_t i = 0; i < count; ++i) {
        if (haystack.find(needles[i]) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void appendLayer(std::vector<IntegrityLayer>* out, IntegrityLayer layer) {
    if (out == nullptr) {
        return;
    }
    for (size_t i = 0; i < out->size(); ++i) {
        if ((*out)[i] == layer) {
            return;
        }
    }
    out->push_back(layer);
}

void appendStrategy(std::vector<IntegrityBypassStrategy>* out, IntegrityBypassStrategy strategy) {
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

bool productLikelyMie(const std::string& productType, const DeviceTreeSummary& summary) {
    if (productType.find("iPhone17,") == 0 || productType.find("iPhone18,") == 0) {
        return true;
    }
    if (summary.socName.find("A18") != std::string::npos ||
        summary.socName.find("A19") != std::string::npos) {
        return true;
    }
    return false;
}

bool inferArm64eProduct(const std::string& productType) {
    if (productType.find("iPhone11,") == 0 || productType.find("iPhone12,") == 0 ||
        productType.find("iPhone13,") == 0 || productType.find("iPhone14,") == 0 ||
        productType.find("iPhone15,") == 0 || productType.find("iPhone16,") == 0 ||
        productType.find("iPhone17,") == 0) {
        return true;
    }
    if (productType.find("iPad8,") == 0 || productType.find("iPad11,") == 0 ||
        productType.find("iPad12,") == 0 || productType.find("iPad13,") == 0) {
        return true;
    }
    return false;
}

} /* anonymous */

const char* integrityLayerToString(IntegrityLayer layer) {
    switch (layer) {
        case IntegrityLayer::None:
            return "none";
        case IntegrityLayer::PacCode:
            return "PAC-code";
        case IntegrityLayer::PacData:
            return "PAC-data";
        case IntegrityLayer::MieData:
            return "MIE-data";
    }
    return "unknown";
}

const char* integrityBypassStrategyToString(IntegrityBypassStrategy strategy) {
    switch (strategy) {
        case IntegrityBypassStrategy::BadRecoveryDelegate:
            return "badRecovery-delegate";
        case IntegrityBypassStrategy::PacSigningGadget:
            return "PAC-signing-gadget";
        case IntegrityBypassStrategy::PtrauthStripPreBoot:
            return "ptrauth-strip-preboot";
        case IntegrityBypassStrategy::KernelPacDisablePatch:
            return "kernel-PAC-disable-patch";
        case IntegrityBypassStrategy::MieResearchDelegate:
            return "MIE-research-delegate";
        case IntegrityBypassStrategy::AmfiBypassHookPath:
            return "AMFI-bypass-then-hook";
    }
    return "unknown";
}

IntegrityProfile buildIntegrityProfile(const DeviceTreeSummary& summary,
                                       const std::string& productType,
                                       const std::string& iosVersion,
                                       bool arm64eHint,
                                       const std::string& kernelcachePath) {
    IntegrityProfile profile;
    profile.arm64e = arm64eHint || inferArm64eProduct(productType);
    profile.kernelPacStrings = scanKernelcache(kernelcachePath, kPacNeedles,
                                               sizeof(kPacNeedles) / sizeof(kPacNeedles[0]));
    profile.kernelMieStrings = scanKernelcache(kernelcachePath, kMieNeedles,
                                              sizeof(kMieNeedles) / sizeof(kMieNeedles[0]));

    const bool ios15Plus =
        !iosVersion.empty() && primitives::iosVersionInRange(iosVersion, "15.0", "99.0");
    const bool ios18Plus =
        !iosVersion.empty() && primitives::iosVersionInRange(iosVersion, "18.0", "99.0");

    if (profile.arm64e || profile.kernelPacStrings) {
        profile.pacCodeRequired = true;
        profile.pacDataRequired = true;
        appendLayer(&profile.layers, IntegrityLayer::PacCode);
        appendLayer(&profile.layers, IntegrityLayer::PacData);
    }

    if (productLikelyMie(productType, summary) || profile.kernelMieStrings ||
        (ios18Plus && profile.arm64e)) {
        profile.mieDataRequired = true;
        appendLayer(&profile.layers, IntegrityLayer::MieData);
    }

    if (profile.pacCodeRequired) {
        appendStrategy(&profile.strategies, IntegrityBypassStrategy::BadRecoveryDelegate);
        appendStrategy(&profile.strategies, IntegrityBypassStrategy::PacSigningGadget);
        appendStrategy(&profile.strategies, IntegrityBypassStrategy::KernelPacDisablePatch);
        profile.recommendedModule = "badRecovery";
        profile.recommendedEnvKey = "PURPLEPOIS0N_DOPAMINE_BADRECOVERY";
    }

    if (profile.mieDataRequired) {
        appendStrategy(&profile.strategies, IntegrityBypassStrategy::MieResearchDelegate);
        appendStrategy(&profile.strategies, IntegrityBypassStrategy::AmfiBypassHookPath);
        if (profile.recommendedModule.empty()) {
            profile.recommendedModule = "MIE-research";
            profile.recommendedEnvKey = "PURPLEPOIS0N_MIE_BYPASS";
        }
    }

    if (!profile.arm64e && !ios15Plus && profile.layers.empty()) {
        appendStrategy(&profile.strategies, IntegrityBypassStrategy::PtrauthStripPreBoot);
        profile.summary = "No PAC/MIE layers — classic arm64 path; hooks via KPF or direct patch.";
        return profile;
    }

    if (profile.pacCodeRequired && profile.mieDataRequired) {
        profile.summary =
            "arm64e PAC (code+data keys) plus MIE-class data integrity — bypass PAC first "
            "(badRecovery), then data-integrity layer before stable kernel hooks.";
    } else if (profile.pacCodeRequired) {
        profile.summary =
            "arm64e PAC required — use badRecovery delegate or PAC signing gadgets before "
            "corrupting signed pointers.";
    } else if (profile.mieDataRequired) {
        profile.summary =
            "Data integrity (MIE) likely — kernel data edits need MIE bypass in addition to "
            "any PAC path.";
    } else {
        profile.summary = "Integrity bypass may be optional on this build.";
    }

    if (!productType.empty()) {
        profile.summary += " product=" + productType;
    }
    return profile;
}

IntegrityProfile buildIntegrityProfileFromContext(const primitives::ExecutionContext& context) {
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

    return buildIntegrityProfile(summary, context.productType, context.iosVersion, context.arm64e,
                                kernelPath);
}

bool pacBypassRequired(const IntegrityProfile& profile) {
    return profile.pacCodeRequired || profile.pacDataRequired;
}

bool dataIntegrityBypassRequired(const IntegrityProfile& profile) {
    return profile.mieDataRequired || profile.pacDataRequired;
}

std::string recommendedIntegrityBypassEnv(const IntegrityProfile& profile) {
    if (!profile.recommendedEnvKey.empty()) {
        const std::string fromEnv = envOrEmpty(profile.recommendedEnvKey.c_str());
        if (!fromEnv.empty()) {
            return fromEnv;
        }
    }
    if (profile.pacCodeRequired) {
        return envOrEmpty("PURPLEPOIS0N_INTEGRITY_BYPASS");
    }
    return std::string();
}

void logIntegrityBypassPlan(const IntegrityProfile& profile) {
    Logger::info(std::string("  [Integrity] arm64e=") + (profile.arm64e ? "yes" : "no") +
                 " PAC=" + (profile.pacCodeRequired ? "required" : "optional") +
                 " data/MIE=" + (profile.mieDataRequired ? "required" : "optional"));
    if (profile.kernelPacStrings) {
        Logger::info("  [Integrity] kernelcache contains ptrauth/PAC symbols");
    }
    if (profile.kernelMieStrings) {
        Logger::info("  [Integrity] kernelcache contains MIE/data-integrity symbols");
    }
    Logger::info("  [Integrity] " + profile.summary);
    for (size_t i = 0; i < profile.layers.size(); ++i) {
        Logger::info(std::string("  [Integrity]   layer: ") +
                     integrityLayerToString(profile.layers[i]));
    }
    for (size_t i = 0; i < profile.strategies.size(); ++i) {
        Logger::info(std::string("  [Integrity]   strategy: ") +
                     integrityBypassStrategyToString(profile.strategies[i]));
    }
    if (!profile.recommendedModule.empty()) {
        Logger::info("  [Integrity]   easy bypass: set " + profile.recommendedEnvKey +
                     " to Dopamine badRecovery .framework (or make plugins + --bypass-integrity)");
    }
}

} /* namespace devicetree */
} /* namespace PP */
