/*
 * PongoProbePrimitive.cpp
 */

#include "primitives/PongoProbePrimitive.h"
#include "primitives/PongoDelegate.h"
#include "pongo/PongoDevice.h"
#include "EnvUtil.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* PongoProbePrimitive::name() const { return "pongo-probe"; }

PrimitiveCategory PongoProbePrimitive::category() const {
    return PrimitiveCategory::Bootrom;
}

std::vector<PrimitiveOperation> PongoProbePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe};
}

std::vector<DeviceState> PongoProbePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::DFU, DeviceState::Unknown};
}

bool PongoProbePrimitive::canRun(const ExecutionContext& context) const {
    if (context.pongoProbeRun) {
        return true;
    }
    return PP::envFlagEnabled("PURPLEPOIS0N_PONGO_PROBE");
}

PrimitiveResult PongoProbePrimitive::execute(ExecutionContext& context) {
    Logger::info("  [Pongo] USB probe (VID 05ac PID 4141 — PongoOS shell)");

    if (!pongoLibusbAvailable()) {
        Logger::warn("  [Pongo] libusb not linked — brew install libusb && rebuild");
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (context.pongoSpawnCheckra1n) {
        const PrimitiveResult spawn = PongoDelegate::spawnCheckra1nShell(context.allowMutation);
        if (spawn != PrimitiveResult::Success) {
            return spawn;
        }
    }

    if (PongoDelegate::isPongoPresent()) {
        Logger::info("  [Pongo] device detected on USB");
    } else {
        Logger::warn("  [Pongo] no PongoOS USB device — run checkra1n -cp or --pongo-spawn-checkra1n");
        return PrimitiveResult::PrerequisitesMissing;
    }

    std::string shellOut;
    const PrimitiveResult shell = PongoDelegate::probeShell(&shellOut);
    if (shell == PrimitiveResult::Success && !shellOut.empty()) {
        Logger::info("  [Pongo] shell: " + shellOut);
    } else if (shell != PrimitiveResult::Success && context.allowMutation) {
        Logger::warn("  [Pongo] shell probe failed (device may still be present)");
    }

    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
