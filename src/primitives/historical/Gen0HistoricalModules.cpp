/*
 * Gen0HistoricalModules.cpp
 */

#include "primitives/historical/Gen0HistoricalModules.h"
#include "primitives/historical/HistoricalExploitCommon.h"

#include "DeviceState.h"

namespace PP {
namespace primitives {

namespace {

const HistoricalExploitDesc kLimera1nDesc = {
    "gen0-limera1n",
    ExploitModuleId::Limera1n,
    900,
    JailbreakGeneration::Gen0,
    "https://github.com/geohot/limera1n (historical)",
    "legacy/Chronic-Dev/syringe/exploits/limera1n (study only)",
    "  [Gen0]   A4-class bootrom — delegate via PURPLEPOIS0N_LIMERA1N",
};

const HistoricalExploitDesc kKpwn24kDesc = {
    "gen0-24kpwn",
    ExploitModuleId::Kpwn24k,
    880,
    JailbreakGeneration::Gen0,
    "https://theapplewiki.com/wiki/0x24000_Segment_Overflow (planetbeing / Chronic Dev era)",
    "legacy/Chronic-Dev/syringe + redsn0w bundles (IMG3 0x24000 layout — study only)",
    "  [Gen0]   old-BR 3GS / iPod 2G untether — PURPLEPOIS0N_24KPWN delegate; pairs with limera1n",
};

} /* anonymous */

Limera1nExploitModule::Limera1nExploitModule() : HistoricalExploitModuleBase(kLimera1nDesc) {}

std::vector<DeviceState> Limera1nExploitModule::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::DFU};
}

bool Limera1nExploitModule::canRun(const ExecutionContext& context) const {
    return context.deviceState == DeviceState::DFU;
}

bool Limera1nExploitModule::supportsContext(const ExecutionContext& context) const {
    return cpidIsPreCheckm8Dfu(context.cpid);
}

Kpwn24kExploitModule::Kpwn24kExploitModule() : HistoricalExploitModuleBase(kKpwn24kDesc) {}

std::vector<DeviceState> Kpwn24kExploitModule::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::DFU};
}

bool Kpwn24kExploitModule::canRun(const ExecutionContext& context) const {
    return context.deviceState == DeviceState::DFU;
}

bool Kpwn24kExploitModule::supportsContext(const ExecutionContext& context) const {
    if (context.cpid == 0) {
        return true;
    }
    return cpidIsOldBootrom24k(context.cpid);
}

} /* namespace primitives */
} /* namespace PP */
