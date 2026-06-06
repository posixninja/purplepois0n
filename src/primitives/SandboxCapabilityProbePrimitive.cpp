/*
 * SandboxCapabilityProbePrimitive.cpp
 */

#include "primitives/SandboxCapabilityProbePrimitive.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* SandboxCapabilityProbePrimitive::name() const {
    return "sandbox-capability-probe";
}

std::vector<PrimitiveOperation> SandboxCapabilityProbePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe};
}

std::vector<DeviceState> SandboxCapabilityProbePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Unknown,
                                    DeviceState::Normal,
                                    DeviceState::Recovery,
                                    DeviceState::DFU};
}

bool SandboxCapabilityProbePrimitive::canRun(const ExecutionContext& /*context*/) const {
    return true;
}

PrimitiveResult SandboxCapabilityProbePrimitive::execute(ExecutionContext& context) {
    Logger::info("  [Sandbox] container/profile escape — NOT in-tree (study public write-ups only)");
    Logger::info("  [Sandbox] backup-mediated staging (absinthe) — parse-only via --analyze-backup");

    if (context.deviceState == DeviceState::Normal) {
        Logger::info("  [Sandbox] AFC reachability probed separately (afc-injection-probe)");
    }

    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
