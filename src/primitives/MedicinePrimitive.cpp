/*
 * MedicinePrimitive.cpp
 */

#include "primitives/MedicinePrimitive.h"
#include "primitives/MedicineDelegate.h"
#include "EnvUtil.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* MedicinePrimitive::name() const { return "medicine-postjb"; }

PrimitiveCategory MedicinePrimitive::category() const {
    return PrimitiveCategory::Bootstrap;
}

std::vector<PrimitiveOperation> MedicinePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> MedicinePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Normal, DeviceState::Unknown};
}

bool MedicinePrimitive::canRun(const ExecutionContext& context) const {
    if (context.medicineRun || context.medicineApply) {
        return true;
    }
    return PP::envFlagEnabled("PURPLEPOIS0N_MEDICINE_PROBE") ||
           PP::envFlagEnabled("PURPLEPOIS0N_MEDICINE_APPLY");
}

PrimitiveResult MedicinePrimitive::execute(ExecutionContext& context) {
    Logger::info("  [Medicine] post-install cures (afc2 / capable / sachet / loader)");
    const bool apply = context.medicineApply || context.allowMutation ||
                       PP::envFlagEnabled("PURPLEPOIS0N_MEDICINE_APPLY");
    return MedicineDelegate::runMedicine(context, apply);
}

} /* namespace primitives */
} /* namespace PP */
