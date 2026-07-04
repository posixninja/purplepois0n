/*
 * PongoWorkflow.cpp
 */

#include "pongo/PongoWorkflow.h"
#include "Gen0Context.h"
#include "primitives/ChainRunner.h"
#include "Logger.h"

namespace PP {

bool runPongoProbe(bool spawnCheckra1n, bool allowMutation, std::string* message) {
    Logger::info("PongoOS USB probe (checkra1n secondary bootloader)");

    primitives::ChainRunner runner;
    primitives::ExecutionContext ctx;
    ctx.deviceState = DeviceState::Unknown;
    ctx.pongoProbeRun = true;
    ctx.pongoSpawnCheckra1n = spawnCheckra1n;
    ctx.allowMutation = allowMutation;

    const bool ok = runner.runPongoMiniChain(ctx, allowMutation);
    if (message != nullptr) {
        *message = ok ? "PongoOS device reachable" : "PongoOS probe failed";
    }
    return ok;
}

bool runPongoBoot(const Gen0Options& options, bool allowMutation) {
    Logger::info("PongoOS KPF + ramdisk boot chain");

    primitives::ChainRunner runner;
    primitives::ExecutionContext ctx =
        buildExecutionContext(DeviceState::Unknown, options, "", 0, allowMutation);
    ctx.pongoBootRun = true;

    const bool ok = runner.runPongoMiniChain(ctx, allowMutation);
    if (!options.reportPath.empty()) {
        if (!runner.writeReportToFile(options.reportPath)) {
            Logger::warn("Failed to write chain report: " + options.reportPath);
        }
    }
    return ok;
}

} /* namespace PP */
