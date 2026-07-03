/*
 * DeviceTreeMmioPrimitive.cpp
 */

#include "primitives/DeviceTreeMmioPrimitive.h"
#include "devicetree/DeviceTreeCatalog.h"
#include "EnvUtil.h"
#include "Logger.h"

#include <cstdlib>

namespace PP {
namespace primitives {

const char* DeviceTreeMmioPrimitive::name() const { return "devicetree-mmio"; }

PrimitiveCategory DeviceTreeMmioPrimitive::category() const {
    return PrimitiveCategory::PhysRw;
}

std::vector<PrimitiveOperation> DeviceTreeMmioPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe};
}

std::vector<DeviceState> DeviceTreeMmioPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Unknown, DeviceState::Normal, DeviceState::Recovery,
                                      DeviceState::DFU};
}

bool DeviceTreeMmioPrimitive::canRun(const ExecutionContext& /*context*/) const { return true; }

PrimitiveResult DeviceTreeMmioPrimitive::execute(ExecutionContext& context) {
    std::string inputPath = context.ipswPath;
    if (inputPath.empty()) {
        inputPath = envOrEmpty("PURPLEPOIS0N_IPSW");
    }
    if (inputPath.empty()) {
        inputPath = envOrEmpty("PURPLEPOIS0N_DTREE_INPUT");
    }

    const bool includeAll = envFlagEnabled("PURPLEPOIS0N_MMIO_ALL");
    devicetree::DeviceTreeCatalog catalog;
    if (!inputPath.empty()) {
        catalog = devicetree::buildCatalogFromPath(inputPath, includeAll);
    } else {
        const std::string jsonPath = envOrEmpty("PURPLEPOIS0N_MMIO_CATALOG");
        if (!jsonPath.empty()) {
            catalog = devicetree::buildCatalogFromJsonFile(jsonPath, includeAll);
        } else {
            Logger::error("  [DeviceTree] set --ipsw, PURPLEPOIS0N_IPSW, or PURPLEPOIS0N_DTREE_INPUT");
            return PrimitiveResult::PrerequisitesMissing;
        }
    }

    devicetree::logCatalogSummary(catalog);
    devicetree::logAgxMmioHints(catalog);
    if (!catalog.success) {
        return PrimitiveResult::Failed;
    }

    devicetree::setGlobalCatalog(catalog);

    if (envFlagEnabled("PURPLEPOIS0N_DTREE_REGISTERS")) {
        devicetree::logFullRegisterInventory(catalog, 0);
    }

    const char* outEnv = std::getenv("PURPLEPOIS0N_MMIO_CATALOG_OUT");
    if (outEnv != nullptr && outEnv[0] != '\0') {
        std::string writeError;
        if (!devicetree::writeCatalogJson(catalog, outEnv, &writeError)) {
            Logger::warn("  [DeviceTree] export failed: " + writeError);
        } else {
            Logger::info("  [DeviceTree] wrote " + std::string(outEnv));
        }
    }

    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
