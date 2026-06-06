/*
 * PongoTypes.cpp
 */

#include "primitives/pongo/PongoTypes.h"
#include "EnvUtil.h"

namespace PP {
namespace primitives {

PongoOptions fillPongoOptionsFromContext(const ExecutionContext& context) {
    PongoOptions opts;
    opts.probeRun = context.pongoProbeRun;
    opts.bootRun = context.pongoBootRun;
    opts.spawnCheckra1n = context.pongoSpawnCheckra1n;
    opts.kpfPath = context.pongoKpfPath;
    opts.ramdiskDmgPath = context.pongoRamdiskDmgPath;
    opts.xargsLine = context.pongoXargsLine;
    return opts;
}

void applyPongoOptionsToContext(ExecutionContext& context, const PongoOptions& options) {
    context.pongoProbeRun = options.probeRun;
    context.pongoBootRun = options.bootRun;
    context.pongoSpawnCheckra1n = options.spawnCheckra1n;
    context.pongoKpfPath = options.kpfPath;
    context.pongoRamdiskDmgPath = options.ramdiskDmgPath;
    context.pongoXargsLine = options.xargsLine;
}

} /* namespace primitives */
} /* namespace PP */
