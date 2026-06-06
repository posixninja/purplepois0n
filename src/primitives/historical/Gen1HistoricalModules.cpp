/*
 * Gen1HistoricalModules.cpp
 */

#include "primitives/historical/Gen1HistoricalModules.h"

namespace PP {
namespace primitives {

namespace {

const HistoricalExploitDesc kEvasi0nDesc = {
    "gen1-evasi0n",
    ExploitModuleId::Evasi0n,
    850,
    JailbreakGeneration::Gen1to4,
    "https://github.com/evad3rs/evasi0n (historical)",
    "OpenJailbreak / evasi0n7 lineage (study only)",
    "  [Gen1]   userland-first untether era — PURPLEPOIS0N_EVASI0N delegate",
};

} /* anonymous */

Evasi0nExploitModule::Evasi0nExploitModule() : HistoricalExploitModuleBase(kEvasi0nDesc) {}

bool Evasi0nExploitModule::supportsContext(const ExecutionContext& context) const {
    if (context.iosVersion.empty()) {
        return true;
    }
    return iosVersionInRange(context.iosVersion, "6.0", "7.0.6") ||
           iosVersionInRange(context.iosVersion, "7.0", "7.0.6");
}

} /* namespace primitives */
} /* namespace PP */
