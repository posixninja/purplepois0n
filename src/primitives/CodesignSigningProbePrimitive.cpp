/*
 * CodesignSigningProbePrimitive.cpp
 */

#include "primitives/CodesignSigningProbePrimitive.h"
#include "primitives/CodesignDelegate.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* CodesignSigningProbePrimitive::name() const { return "codesign-signing-probe"; }

PrimitiveCategory CodesignSigningProbePrimitive::category() const {
    return PrimitiveCategory::Codesign;
}

std::vector<PrimitiveOperation> CodesignSigningProbePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe};
}

std::vector<DeviceState> CodesignSigningProbePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Unknown,
                                    DeviceState::Normal,
                                    DeviceState::Recovery,
                                    DeviceState::DFU};
}

bool CodesignSigningProbePrimitive::canRun(const ExecutionContext& context) const {
    if (!context.codesignInputPath.empty() || !context.ipaInstallPath.empty()) {
        return true;
    }
    return CodesignDelegate::isIpswConfigured() || CodesignDelegate::isLdidConfigured();
}

PrimitiveResult CodesignSigningProbePrimitive::execute(ExecutionContext& context) {
    Logger::info("  [Codesign] host sign / sideload tool probe");
    return CodesignDelegate::probe(context);
}

} /* namespace primitives */
} /* namespace PP */
