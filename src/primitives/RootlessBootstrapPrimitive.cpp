/*
 * RootlessBootstrapPrimitive.cpp
 */

#include "primitives/RootlessBootstrapPrimitive.h"
#include "primitives/RootlessDelegate.h"
#include "primitives/RootlessLayout.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* RootlessBootstrapPrimitive::name() const { return "rootless-bootstrap"; }

PrimitiveCategory RootlessBootstrapPrimitive::category() const {
    return PrimitiveCategory::Bootstrap;
}

std::vector<PrimitiveOperation> RootlessBootstrapPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> RootlessBootstrapPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Normal, DeviceState::Unknown};
}

bool RootlessBootstrapPrimitive::canRun(const ExecutionContext& context) const {
    if (RootlessDelegate::preferRootless(context)) {
        return true;
    }
    if (RootlessDelegate::sshConfigured(context)) {
        return true;
    }
    const char* fixture = std::getenv("PURPLEPOIS0N_JBROOT_FIXTURE");
    return fixture != nullptr && fixture[0] != '\0';
}

PrimitiveResult RootlessBootstrapPrimitive::execute(ExecutionContext& context) {
    Logger::info("  [Rootless] sealed system volume — bootstrap under " +
                 RootlessLayout::resolveJbroot());
    Logger::info("  [Rootless] host maps rootfs paths via jbPath(); no / remount");

    const char* fixture = std::getenv("PURPLEPOIS0N_JBROOT_FIXTURE");
    if (fixture != nullptr && fixture[0] != '\0') {
        const RootlessProbeResult local = RootlessLayout::probeLocalTree(fixture);
        RootlessDelegate::logProbeResult(local);
        if (local.layoutComplete) {
            return PrimitiveResult::Success;
        }
        return local.jbrootExists ? PrimitiveResult::ProbeOnly : PrimitiveResult::Failed;
    }

    if (!RootlessDelegate::sshConfigured(context)) {
        Logger::info("  [Rootless] offline probe — set PURPLEPOIS0N_JBROOT_FIXTURE for local tree");
        Logger::info("  [Rootless] on-device: forward SSH (iproxy 2222 22) + --normal-ssh");
        if (RootlessDelegate::jbHelperConfigured() ||
            RootlessDelegate::palera1nHelperConfigured()) {
            Logger::info("  [Rootless] bootstrap helper env configured (delegate on --execute)");
        }
        return PrimitiveResult::Success;
    }

    const RootlessProbeResult remote = RootlessDelegate::probeDevice(context);
    RootlessDelegate::logProbeResult(remote);

    if (!remote.sshReachable) {
        return PrimitiveResult::TransportError;
    }
    if (!remote.jbrootExists) {
        Logger::warn("  [Rootless] jbroot missing — jailbreak bootstrap not installed");
        if (context.allowMutation) {
            const PrimitiveResult helper = RootlessDelegate::runBootstrapHelper(context, true);
            if (helper != PrimitiveResult::PrerequisitesMissing) {
                return helper;
            }
        }
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (context.allowMutation && !remote.layoutComplete) {
        const PrimitiveResult helper = RootlessDelegate::runBootstrapHelper(context, true);
        if (helper != PrimitiveResult::PrerequisitesMissing) {
            return helper;
        }
    }

    return remote.layoutComplete ? PrimitiveResult::Success : PrimitiveResult::ProbeOnly;
}

} /* namespace primitives */
} /* namespace PP */
