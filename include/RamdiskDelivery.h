/*
 * RamdiskDelivery.h
 *
 * Lane-agnostic ramdisk artifact resolution. Boot modules (KPF, etc.) are optional
 * and independent — see resolveBootModulePath().
 */

#ifndef RAMDISK_DELIVERY_H_
#define RAMDISK_DELIVERY_H_

#include "RamdiskTypes.h"
#include "primitives/PrimitiveTypes.h"

#include <string>
#include <vector>

namespace PP {

/** Merge CLI, env, and legacy aliases into one delivery spec. */
BootDeliverySpec resolveBootDelivery(const primitives::ExecutionContext& context);

/** @deprecated Use resolveBootDelivery */
inline BootDeliverySpec resolveRamdiskDelivery(const primitives::ExecutionContext& context) {
    return resolveBootDelivery(context);
}

/** Resolve ramdisk artifact only; may pack from IPSW. Does not require a boot module. */
bool resolveRamdiskArtifactPath(primitives::ExecutionContext& context, std::string* outPath);

/** Optional boot module (--boot-module). Empty when unset. */
std::string resolveBootModulePath(const primitives::ExecutionContext& context);

/** Optional boot-args line. Empty when unset (transport applies its default). */
std::string resolveBootArgsLine(const primitives::ExecutionContext& context);

/** Default boot module from env / built tree (KPF today). */
std::string resolveDefaultBootModulePath();

RamdiskArtifactFormat inferArtifactFormat(const std::string& path,
                                          RamdiskArtifactFormat hint);

bool readBootArtifactBytes(const std::string& path, std::vector<uint8_t>* out);

/** True when boot delivery probe or execute was requested (any lane). */
bool bootDeliveryRequested(const primitives::ExecutionContext& context);

} /* namespace PP */

#endif /* RAMDISK_DELIVERY_H_ */
