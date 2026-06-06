/*
 * TssSigningProbePrimitive.cpp
 */

#include "primitives/TssSigningProbePrimitive.h"
#include "primitives/TssDelegate.h"
#include "primitives/TssTypes.h"
#include "Logger.h"

#include <cstdlib>

namespace PP {
namespace primitives {

const char* TssSigningProbePrimitive::name() const { return "tss-signing-probe"; }

PrimitiveCategory TssSigningProbePrimitive::category() const {
    return PrimitiveCategory::Codesign;
}

std::vector<PrimitiveOperation> TssSigningProbePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe};
}

std::vector<DeviceState> TssSigningProbePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Unknown,
                                    DeviceState::Normal,
                                    DeviceState::Recovery,
                                    DeviceState::DFU};
}

bool TssSigningProbePrimitive::canRun(const ExecutionContext& context) const {
    if (!context.ipswPath.empty() || !context.apticketPath.empty()) {
        return true;
    }
    if (TssDelegate::isIdevicerestoreConfigured() || TssDelegate::isFuturerestoreConfigured() ||
        TssDelegate::isIpswConfigured()) {
        return true;
    }
    const char* ticket = std::getenv("PURPLEPOIS0N_APTICKET");
    return ticket != nullptr && ticket[0] != '\0';
}

PrimitiveResult TssSigningProbePrimitive::execute(ExecutionContext& context) {
    Logger::info("  [TSS] signing / restore ticket probe");
    const PrimitiveResult probeResult = TssDelegate::probe(context);
    if (probeResult != PrimitiveResult::Success) {
        return probeResult;
    }
    if (!context.ipswPath.empty() &&
        TssDelegate::resolveSigningMode(context) == TssSigningMode::SavedApTicket) {
        if (!TssDelegate::isFuturerestoreConfigured()) {
            Logger::error("  [TSS] futurerestore required for saved-blob restore planning");
            return PrimitiveResult::PrerequisitesMissing;
        }
        TssDelegate::runFuturerestoreRestore(context, false);
    }
    return probeResult;
}

} /* namespace primitives */
} /* namespace PP */
