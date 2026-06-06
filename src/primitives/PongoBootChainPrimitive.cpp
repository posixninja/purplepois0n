/*
 * PongoBootChainPrimitive.cpp
 */

#include "primitives/PongoBootChainPrimitive.h"
#include "primitives/PongoDelegate.h"
#include "pongo/PongoDevice.h"
#include "pongo/PongoBootHelpers.h"
#include "EnvUtil.h"
#include "Logger.h"

#include <vector>

namespace PP {
namespace primitives {

const char* PongoBootChainPrimitive::name() const { return "pongo-boot-chain"; }

PrimitiveCategory PongoBootChainPrimitive::category() const {
    return PrimitiveCategory::Injection;
}

std::vector<PrimitiveOperation> PongoBootChainPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> PongoBootChainPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::DFU, DeviceState::Unknown};
}

bool PongoBootChainPrimitive::canRun(const ExecutionContext& context) const {
    if (context.pongoBootRun) {
        return true;
    }
    return PP::envFlagEnabled("PURPLEPOIS0N_PONGO_BOOT");
}

PrimitiveResult PongoBootChainPrimitive::execute(ExecutionContext& context) {
    Logger::info("  [Pongo] KPF + ramdisk boot (checkra1n boot-checkra1n.py sequence)");

    if (!pongoLibusbAvailable()) {
        Logger::warn("  [Pongo] libusb not linked — brew install libusb && rebuild");
        return PrimitiveResult::PrerequisitesMissing;
    }

    const std::string kpfPath = resolvePongoKpfPath(context);
    if (kpfPath.empty()) {
        Logger::warn("  [Pongo] KPF path required — --pongo-kpf or PURPLEPOIS0N_KPF");
        return PrimitiveResult::PrerequisitesMissing;
    }

    std::string dmgPath;
    if (!resolvePongoRamdiskDmg(context, &dmgPath)) {
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (!context.allowMutation) {
        Logger::info("  [Pongo] would upload KPF: " + kpfPath);
        Logger::info("  [Pongo] would upload ramdisk DMG: " + dmgPath);
        Logger::info("  [Pongo] would run modload → ramdisk → bootx");
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [Pongo] boot upload requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    if (context.pongoSpawnCheckra1n) {
        const PrimitiveResult spawn = PongoDelegate::spawnCheckra1nShell(true);
        if (spawn != PrimitiveResult::Success) {
            return spawn;
        }
    }

    if (!PongoDelegate::isPongoPresent()) {
        Logger::error("  [Pongo] no PongoOS device — run checkra1n -cp first");
        return PrimitiveResult::PrerequisitesMissing;
    }

    std::vector<uint8_t> kpf;
    std::vector<uint8_t> rdsk;
    if (!readFileBytes(kpfPath, &kpf) || !readFileBytes(dmgPath, &rdsk)) {
        return PrimitiveResult::Failed;
    }

    PongoDevice dev;
    if (!dev.open()) {
        return PrimitiveResult::TransportError;
    }

    const std::string xargs = resolvePongoXargs(context);
    if (!dev.bootCheckra1nSequence(kpf, rdsk, xargs)) {
        return PrimitiveResult::Failed;
    }

    Logger::info("  [Pongo] boot sequence complete");
    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
