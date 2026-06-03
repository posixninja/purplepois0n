/*
 * NormalModeProbePrimitive.cpp
 */

#include "primitives/NormalModeProbePrimitive.h"
#include "MobileDevice.h"
#include "Logger.h"

#include <sstream>

namespace PP {
namespace primitives {

const char* NormalModeProbePrimitive::name() const {
    return "normal-mode-probe";
}

std::vector<PrimitiveOperation> NormalModeProbePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe};
}

std::vector<DeviceState> NormalModeProbePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Normal};
}

bool NormalModeProbePrimitive::canRun(const ExecutionContext& context) const {
    return context.deviceState == DeviceState::Normal && !context.udid.empty();
}

std::string NormalModeProbePrimitive::deliveryPath() const {
    return "lockdown";
}

PrimitiveResult NormalModeProbePrimitive::execute(ExecutionContext& context) {
    try {
        MobileDevice device(context.udid);
        const std::vector<std::string> apps = device.getInstalledApplications();
        std::ostringstream oss;
        oss << "installed apps: " << apps.size();
        Logger::info(std::string("  [Normal] ") + oss.str());
        return PrimitiveResult::Success;
    } catch (const std::exception& e) {
        Logger::warn(std::string("  [Normal] lockdown probe failed: ") + e.what());
        return PrimitiveResult::PrerequisitesMissing;
    }
}

} /* namespace primitives */
} /* namespace PP */
