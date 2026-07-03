/*
 * HostKernelPatchPrimitive.cpp
 */

#include "primitives/HostKernelPatchPrimitive.h"
#include "primitives/HostPatchEngine.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* HostKernelPatchPrimitive::name() const { return "host-kernel-patch"; }

PrimitiveCategory HostKernelPatchPrimitive::category() const {
    return PrimitiveCategory::Patchfinding;
}

std::vector<PrimitiveOperation> HostKernelPatchPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe,
                                           PrimitiveOperation::Patch,
                                           PrimitiveOperation::Overwrite};
}

std::vector<DeviceState> HostKernelPatchPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Unknown,
                                    DeviceState::Normal,
                                    DeviceState::Recovery,
                                    DeviceState::DFU};
}

bool HostKernelPatchPrimitive::canRun(const ExecutionContext& context) const {
    if (!resolveKernelcachePath(context).empty()) {
        return true;
    }
    if (!resolvePatchProfilePath(context).empty()) {
        return true;
    }
    return !context.ipswPath.empty();
}

ChainStage HostKernelPatchPrimitive::gen6Stage() const { return ChainStage::Patchfind; }

PrimitiveResult HostKernelPatchPrimitive::execute(ExecutionContext& context) {
    const PrimitiveResult findResult = runHostPatchfind(context);
    if (findResult != PrimitiveResult::Success) {
        return findResult;
    }

    if (!resolvePatchProfilePath(context).empty()) {
        return runHostPatchApply(context);
    }

    if (!context.allowMutation) {
        Logger::info("  [Patch] supply --patch-profile for offline Patch/Overwrite execute");
    }
    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
