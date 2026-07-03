/*
 * RootlessDelegate.h
 *
 * SSH/AFC delegation for rootless bootstrap probe and external installer hooks.
 */

#ifndef PRIMITIVES_ROOTLESS_DELEGATE_H_
#define PRIMITIVES_ROOTLESS_DELEGATE_H_

#include "PrimitiveTypes.h"
#include "RootlessLayout.h"
#include "RamdiskTypes.h"

namespace PP {
namespace primitives {

class RootlessDelegate {
public:
    /** Gen6 / iOS 15+ or PURPLEPOIS0N_ROOTLESS=1. */
    static bool preferRootless(const ExecutionContext& context);

    /** SSH to jailbroken Normal device (--normal-ssh / ramdiskConnect transport=ssh). */
    static bool sshConfigured(const ExecutionContext& context);

    static RamdiskConnectOptions sshConnectOptions(const ExecutionContext& context);

    /** Probe jbroot markers over SSH; probe-only when @p allowMutation is false. */
    static RootlessProbeResult probeDevice(const ExecutionContext& context);

    static void logProbeResult(const RootlessProbeResult& result);

    /** Spawn PURPLEPOIS0N_JB_HELPER or PURPLEPOIS0N_PALERA1N_HELPER with rootless args. */
    static PrimitiveResult runBootstrapHelper(const ExecutionContext& context, bool allowMutation);

    static bool jbHelperConfigured();
    static bool palera1nHelperConfigured();
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_ROOTLESS_DELEGATE_H_ */
