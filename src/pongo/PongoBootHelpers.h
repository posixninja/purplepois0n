/*
 * PongoBootHelpers.h
 */

#ifndef PONGO_BOOT_HELPERS_H_
#define PONGO_BOOT_HELPERS_H_

#include "primitives/PrimitiveTypes.h"

#include <string>
#include <vector>

namespace PP {

std::string resolvePongoKpfPath(const primitives::ExecutionContext& context);
std::string resolvePongoXargs(const primitives::ExecutionContext& context);
bool readFileBytes(const std::string& path, std::vector<uint8_t>* out);
bool resolvePongoRamdiskDmg(primitives::ExecutionContext& context, std::string* dmgPath);

} /* namespace PP */

#endif /* PONGO_BOOT_HELPERS_H_ */
