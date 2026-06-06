/*
 * PongoChain.cpp
 */

#include "primitives/pongo/PongoChain.h"
#include "primitives/ChainRunner.h"
#include "primitives/PrimitiveRegistry.h"
#include "EnvUtil.h"
#include "Logger.h"

namespace PP {
namespace primitives {

bool runPongoChain(ExecutionContext& context, bool executeMode, ChainRunner& runner) {
    if (!context.pongoProbeRun && !context.pongoBootRun) {
        if (!PP::envFlagEnabled("PURPLEPOIS0N_PONGO_PROBE") &&
            !PP::envFlagEnabled("PURPLEPOIS0N_PONGO_BOOT")) {
            return false;
        }
    }

    PrimitiveRegistry& registry = PrimitiveRegistry::instance();
    registry.registerBuiltins();

    const bool savedMutation = context.allowMutation;
    context.allowMutation = executeMode;

    const bool runProbe =
        context.pongoProbeRun || PP::envFlagEnabled("PURPLEPOIS0N_PONGO_PROBE");
    if (runProbe) {
        Primitive* probe = registry.findByName("pongo-probe");
        if (probe != nullptr && probe->canRun(context)) {
            runner.logStage(executeMode ? ChainStage::Execute : ChainStage::Probe,
                            "Pongo: USB shell probe");
            const PrimitiveResult result = probe->execute(context);
            runner.recordReport(ChainStage::Probe, result, "PongoProbePrimitive");
            if (result != PrimitiveResult::Success) {
                context.allowMutation = savedMutation;
                return false;
            }
        }
    }

    const bool runBoot =
        context.pongoBootRun || PP::envFlagEnabled("PURPLEPOIS0N_PONGO_BOOT");
    if (runBoot) {
        Primitive* boot = registry.findByName("pongo-boot-chain");
        if (boot == nullptr) {
            runner.logStage(ChainStage::Probe, "Pongo: pongo-boot-chain primitive missing");
            context.allowMutation = savedMutation;
            return false;
        }
        if (!boot->canRun(context)) {
            runner.logStage(ChainStage::Probe,
                            "Pongo: boot prerequisites missing (--pongo-kpf, ramdisk DMG)");
            runner.recordReport(ChainStage::Probe, PrimitiveResult::PrerequisitesMissing,
                                "pongo-boot-chain");
            context.allowMutation = savedMutation;
            return false;
        }
        runner.logStage(executeMode ? ChainStage::Execute : ChainStage::Probe,
                        "Pongo: KPF + ramdisk → bootx");
        const PrimitiveResult result = boot->execute(context);
        runner.recordReport(executeMode ? ChainStage::Execute : ChainStage::Probe, result,
                            "PongoBootChainPrimitive");
        context.allowMutation = savedMutation;
        return result == PrimitiveResult::Success;
    }

    context.allowMutation = savedMutation;
    return true;
}

} /* namespace primitives */
} /* namespace PP */
