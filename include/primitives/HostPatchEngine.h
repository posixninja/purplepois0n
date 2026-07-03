/*
 * HostPatchEngine.h
 *
 * Host-side kernelcache patchfind and user-supplied patch application (no bundled offsets).
 * Patch profiles may include optional "expect_hex" per entry for pre-apply validation.
 */

#ifndef PRIMITIVES_HOST_PATCH_ENGINE_H_
#define PRIMITIVES_HOST_PATCH_ENGINE_H_

#include "PrimitiveTypes.h"

namespace PP {
namespace primitives {

/** Resolve kernelcache path from context fields or PURPLEPOIS0N_KERNELCACHE. */
std::string resolveKernelcachePath(const ExecutionContext& context);

/** Resolve patch profile JSON from context or PURPLEPOIS0N_PATCH_PROFILE. */
std::string resolvePatchProfilePath(const ExecutionContext& context);

/** Resolve output path from context or PURPLEPOIS0N_PATCH_OUT. */
std::string resolvePatchOutPath(const ExecutionContext& context);

/** Analyze kernelcache with MachOBinary (patchfind probe). */
PrimitiveResult runHostPatchfind(const ExecutionContext& context);

/** Apply user patch profile and write --patch-out (requires allowMutation). */
PrimitiveResult runHostPatchApply(ExecutionContext& context);

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_HOST_PATCH_ENGINE_H_ */
