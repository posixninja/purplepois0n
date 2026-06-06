/*
 * Gen5HistoricalModules.cpp
 */

#include "primitives/historical/Gen5HistoricalModules.h"
#include "primitives/historical/HistoricalExploitCommon.h"
#include "Checkm8.h"
#include "DeviceState.h"

namespace PP {
namespace primitives {

namespace {

const HistoricalExploitDesc kCheckra1nDesc = {
    "gen5-checkra1n",
    ExploitModuleId::Checkra1n,
    950,
    JailbreakGeneration::Gen5,
    "https://checkra.in (checkm8 bootstrap)",
    "external gaster/ipwndfu + checkra1n app (delegate only)",
    "  [Gen5]   checkm8 hardware-assisted — PURPLEPOIS0N_CHECKRA1N or -m/--checkm8",
};

} /* anonymous */

Checkra1nExploitModule::Checkra1nExploitModule() : HistoricalExploitModuleBase(kCheckra1nDesc) {}

std::vector<DeviceState> Checkra1nExploitModule::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::DFU, DeviceState::Normal};
}

bool Checkra1nExploitModule::canRun(const ExecutionContext& context) const {
    if (context.deviceState == DeviceState::Normal) {
        return !context.udid.empty();
    }
    return context.deviceState == DeviceState::DFU;
}

bool Checkra1nExploitModule::supportsContext(const ExecutionContext& context) const {
    if (context.deviceState == DeviceState::DFU) {
        return context.cpid == 0 || Checkm8::isSupportedCpid(context.cpid);
    }
    if (context.iosVersion.empty()) {
        return true;
    }
    return iosVersionInRange(context.iosVersion, "12.0", "14.9");
}

} /* namespace primitives */
} /* namespace PP */
