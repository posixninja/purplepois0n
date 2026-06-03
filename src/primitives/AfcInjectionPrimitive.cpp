/*
 * AfcInjectionPrimitive.cpp
 */

#include "primitives/AfcInjectionPrimitive.h"
#include "AFCService.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* AfcInjectionPrimitive::name() const {
    return "afc-injection";
}

std::vector<PrimitiveOperation> AfcInjectionPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Inject};
}

std::vector<DeviceState> AfcInjectionPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Normal};
}

bool AfcInjectionPrimitive::canRun(const ExecutionContext& context) const {
    return context.deviceState == DeviceState::Normal && !context.udid.empty();
}

std::string AfcInjectionPrimitive::deliveryPath() const {
    return "afc";
}

PrimitiveResult AfcInjectionPrimitive::execute(ExecutionContext& context) {
    if (context.allowMutation) {
        if (!exploitPluginsEnabled()) {
            Logger::warn("  [Injection] Inject blocked — PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS not set");
            return PrimitiveResult::PluginDisabled;
        }
        Logger::info("  [Injection] AFC inject not implemented — use AFCService API directly");
        return PrimitiveResult::ProbeOnly;
    }

    try {
        AFCService afc(context.udid);
        (void)afc;
        Logger::info("  [Injection] AFCService available for device " + context.udid);
        return PrimitiveResult::Success;
    } catch (const std::exception& e) {
        Logger::warn(std::string("  [Injection] AFCService unavailable: ") + e.what());
        return PrimitiveResult::PrerequisitesMissing;
    }
}

} /* namespace primitives */
} /* namespace PP */
