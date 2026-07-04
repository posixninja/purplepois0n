/*
 * BootChain.cpp
 */

#include "primitives/BootChain.h"
#include "primitives/ChainRunner.h"
#include "primitives/PrimitiveRegistry.h"
#include "RamdiskDelivery.h"
#include "EnvUtil.h"
#include "Logger.h"

namespace PP {
namespace primitives {

namespace {

bool bootProbeRequested(const ExecutionContext& context) {
    return context.bootDeliveryProbe || context.pongoProbeRun ||
           PP::envFlagEnabled("PURPLEPOIS0N_BOOT_PROBE") ||
           PP::envFlagEnabled("PURPLEPOIS0N_PONGO_PROBE");
}

bool bootExecuteRequested(const ExecutionContext& context) {
    return context.bootDeliveryRun || context.pongoBootRun ||
           PP::envFlagEnabled("PURPLEPOIS0N_BOOT_DELIVERY") ||
           PP::envFlagEnabled("PURPLEPOIS0N_PONGO_BOOT");
}

const char* probePrimitiveForLane(const BootDeliveryLane lane) {
    switch (lane) {
        case BootDeliveryLane::UsbLoader:
            return "pongo-probe";
        case BootDeliveryLane::LiveAgent:
            return "ramdisk-shell";
        default:
            return nullptr;
    }
}

const char* bootPrimitiveForLane(const BootDeliveryLane lane) {
    switch (lane) {
        case BootDeliveryLane::UsbLoader:
            return "usb-loader-boot-chain";
        case BootDeliveryLane::Recovery:
            return "recovery-boot-chain";
        case BootDeliveryLane::LiveAgent:
            return "ramdisk-shell";
        default:
            return nullptr;
    }
}

BootDeliveryLane effectiveLane(const ExecutionContext& context) {
    const BootDeliverySpec spec = resolveBootDelivery(context);
    return spec.lane;
}

} /* anonymous */

bool runBootDeliveryChain(ExecutionContext& context, bool executeMode, ChainRunner& runner) {
    if (!bootProbeRequested(context) && !bootExecuteRequested(context) &&
        !context.recoveryChainRun) {
        if (!bootDeliveryRequested(context)) {
            return false;
        }
    }

    PrimitiveRegistry& registry = PrimitiveRegistry::instance();
    registry.registerBuiltins();

    const BootDeliveryLane lane = effectiveLane(context);
    const bool savedMutation = context.allowMutation;
    context.allowMutation = executeMode;

    if (bootProbeRequested(context)) {
        const char* probeName = probePrimitiveForLane(lane);
        if (probeName != nullptr) {
            Primitive* probe = registry.findByName(probeName);
            if (probe != nullptr && probe->canRun(context)) {
                runner.logStage(executeMode ? ChainStage::Execute : ChainStage::Probe,
                                std::string("Boot probe (") + bootDeliveryLaneLabel(lane) + ")");
                const PrimitiveResult result = probe->execute(context);
                runner.recordReport(ChainStage::Probe, result, probeName);
                if (result != PrimitiveResult::Success) {
                    context.allowMutation = savedMutation;
                    return false;
                }
            }
        }
    }

    if (bootExecuteRequested(context) || context.recoveryChainRun) {
        const char* bootName = bootPrimitiveForLane(lane);
        if (bootName == nullptr) {
            runner.logStage(ChainStage::Probe,
                            std::string("Boot: lane ") + bootDeliveryLaneLabel(lane) +
                                " not implemented");
            runner.recordReport(ChainStage::Probe, PrimitiveResult::PrerequisitesMissing,
                                "boot-delivery");
            context.allowMutation = savedMutation;
            return false;
        }

        Primitive* boot = registry.findByName(bootName);
        if (boot == nullptr) {
            runner.logStage(ChainStage::Probe,
                            std::string("Boot: primitive missing: ") + bootName);
            context.allowMutation = savedMutation;
            return false;
        }
        if (!boot->canRun(context)) {
            runner.logStage(
                ChainStage::Probe,
                std::string("Boot: prerequisites missing (lane=") + bootDeliveryLaneLabel(lane) +
                    ", --ramdisk, optional --boot-module)");
            runner.recordReport(ChainStage::Probe, PrimitiveResult::PrerequisitesMissing, bootName);
            context.allowMutation = savedMutation;
            return false;
        }
        runner.logStage(executeMode ? ChainStage::Execute : ChainStage::Probe,
                        std::string("Boot delivery (") + bootDeliveryLaneLabel(lane) + ")");
        const PrimitiveResult result = boot->execute(context);
        runner.recordReport(executeMode ? ChainStage::Execute : ChainStage::Probe, result, bootName);
        context.allowMutation = savedMutation;
        return result == PrimitiveResult::Success;
    }

    context.allowMutation = savedMutation;
    return true;
}

} /* namespace primitives */
} /* namespace PP */
