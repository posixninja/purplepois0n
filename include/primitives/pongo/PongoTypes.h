/*
 * PongoTypes.h
 *
 * PongoOS lane options shared by CLI, Gen0, and primitives.
 */

#ifndef PRIMITIVES_PONGO_TYPES_H_
#define PRIMITIVES_PONGO_TYPES_H_

#include "primitives/PrimitiveTypes.h"

#include <string>

namespace PP {
namespace primitives {

struct PongoOptions {
    bool probeRun = false;
    bool bootRun = false;
    bool spawnCheckra1n = false;
    std::string kpfPath;
    std::string ramdiskDmgPath;
    std::string xargsLine;
};

PongoOptions fillPongoOptionsFromContext(const ExecutionContext& context);
void applyPongoOptionsToContext(ExecutionContext& context, const PongoOptions& options);

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_PONGO_TYPES_H_ */
