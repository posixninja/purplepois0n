/*
 * PongoBootHelpers.cpp
 *
 * Legacy wrappers — boot resolution lives in RamdiskDelivery.cpp.
 */

#include "pongo/PongoBootHelpers.h"
#include "RamdiskDelivery.h"

namespace PP {

std::string resolveDefaultKpfPath() {
    return resolveDefaultBootModulePath();
}

std::string resolvePongoKpfPath(const primitives::ExecutionContext& context) {
    return resolveBootModulePath(context);
}

std::string resolvePongoXargs(const primitives::ExecutionContext& context) {
    return resolveBootArgsLine(context);
}

bool readFileBytes(const std::string& path, std::vector<uint8_t>* out) {
    return readBootArtifactBytes(path, out);
}

bool resolvePongoRamdiskDmg(primitives::ExecutionContext& context, std::string* dmgPath) {
    return resolveRamdiskArtifactPath(context, dmgPath);
}

} /* namespace PP */
