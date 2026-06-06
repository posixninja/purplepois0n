/*
 * OfflinePatchPrimitive.cpp
 */

#include "primitives/OfflinePatchPrimitive.h"
#include "primitives/CodesignDelegate.h"
#include "primitives/CodesignTypes.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* OfflinePatchPrimitive::name() const {
    return "offline-codesign-patch";
}

std::vector<PrimitiveOperation> OfflinePatchPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe,
                                           PrimitiveOperation::Patch,
                                           PrimitiveOperation::Overwrite};
}

std::vector<DeviceState> OfflinePatchPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Unknown,
                                    DeviceState::Normal,
                                    DeviceState::Recovery,
                                    DeviceState::DFU};
}

bool OfflinePatchPrimitive::canRun(const ExecutionContext& /*context*/) const {
    return true;
}

std::string OfflinePatchPrimitive::targetKind() const {
    return "kernelcache";
}

PrimitiveResult OfflinePatchPrimitive::execute(ExecutionContext& context) {
    if (!context.codesignInputPath.empty()) {
        CodesignOptions opts =
            CodesignDelegate::mergeOptions(context.codesign, codesignOptionsFromEnv());
        if (!context.codesignOutputPath.empty()) {
            opts.outputPath = context.codesignOutputPath;
        }
        return CodesignDelegate::signPath(opts, context.codesignInputPath, context.allowMutation);
    }

    if (context.allowMutation) {
        if (!exploitPluginsEnabled()) {
            Logger::warn("  [Codesign] Patch/Overwrite blocked — "
                         "PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS not set");
            return PrimitiveResult::PluginDisabled;
        }
        Logger::info("  [Codesign] Offline patch surface available (no bundled patterns in-tree)");
        return PrimitiveResult::ProbeOnly;
    }

    Logger::info("  [Codesign] Offline patch surface available — "
                 "MachOParser / future patch engine (see docs/SUPPORT.md)");
    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
