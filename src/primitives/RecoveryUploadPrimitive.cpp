/*
 * RecoveryUploadPrimitive.cpp
 */

#include "primitives/RecoveryUploadPrimitive.h"
#include "primitives/TssDelegate.h"
#include "primitives/TssTypes.h"
#include "primitives/ITransport.h"
#include "Logger.h"

#include <cstdlib>
#include <stdexcept>
#include <cstring>

namespace PP {
namespace primitives {

const char* RecoveryUploadPrimitive::name() const { return "recovery-upload"; }

PrimitiveCategory RecoveryUploadPrimitive::category() const {
    return PrimitiveCategory::Injection;
}

std::vector<PrimitiveOperation> RecoveryUploadPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> RecoveryUploadPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Recovery};
}

bool RecoveryUploadPrimitive::canRun(const ExecutionContext& context) const {
    if (context.deviceState != DeviceState::Recovery) {
        return false;
    }
    if (!context.recoveryUploadPath.empty() || !context.ipswComponentPath.empty()) {
        return true;
    }
    const char* upload = std::getenv("PURPLEPOIS0N_RECOVERY_UPLOAD");
    return upload != nullptr && upload[0] != '\0';
}

namespace {

std::string resolveUploadPath(const ExecutionContext& context) {
    if (!context.recoveryUploadPath.empty()) {
        return context.recoveryUploadPath;
    }
    const char* env = std::getenv("PURPLEPOIS0N_RECOVERY_UPLOAD");
    return (env != nullptr) ? std::string(env) : std::string();
}

std::string resolveManifest(const ExecutionContext& context) {
    if (!context.im4mManifestPath.empty()) {
        return context.im4mManifestPath;
    }
    const char* env = std::getenv("PURPLEPOIS0N_IM4M_MANIFEST");
    return (env != nullptr) ? std::string(env) : std::string();
}

} /* anonymous */

PrimitiveResult RecoveryUploadPrimitive::execute(ExecutionContext& context) {
    const std::string label =
        context.recoveryComponentLabel.empty() ? "component" : context.recoveryComponentLabel;

    if (context.transport == nullptr || !context.transport->isLive()) {
        Logger::warn("  [Recovery] upload requires RecoveryTransport");
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (std::string(context.transport->transportName()) != "RecoveryTransport") {
        Logger::warn("  [Recovery] upload requires Recovery mode transport");
        return PrimitiveResult::TransportError;
    }

    const std::string nonce = context.transport->getDeviceEnv("NONCE");
    if (!nonce.empty()) {
        Logger::info("  [Recovery] ApNonce (NONCE): " + nonce);
    } else {
        Logger::info("  [Recovery] ApNonce not in iBoot env (futurerestore -w / generator path)");
    }

    std::string uploadPath = resolveUploadPath(context);

    if (uploadPath.empty() && !context.ipswComponentPath.empty()) {
        std::string manifest = resolveManifest(context);
        if (manifest.empty() && !context.apticketPath.empty()) {
            manifest = "/tmp/pp-im4m.im4m";
            if (TssDelegate::extractIm4mFromApticket(context.apticketPath, manifest) !=
                PrimitiveResult::Success) {
                Logger::error("  [Recovery] failed to extract IM4M from APTicket");
                return PrimitiveResult::Failed;
            }
        } else if (manifest.empty() && !context.ipswPath.empty() &&
                   TssDelegate::resolveSigningMode(context) == TssSigningMode::StockLive) {
            const std::string shshTmp = "/tmp/pp-live.shsh";
            manifest = "/tmp/pp-live.im4m";
            if (TssDelegate::fetchLiveIm4m(context, shshTmp, manifest) != PrimitiveResult::Success) {
                Logger::error("  [Recovery] live TSS IM4M fetch failed");
                return PrimitiveResult::Failed;
            }
        }
        if (manifest.empty()) {
            Logger::warn("  [Recovery] need --im4m-manifest or APTicket for personalize");
            return PrimitiveResult::PrerequisitesMissing;
        }

        uploadPath = "/tmp/pp-recovery-signed.img4";
        const std::string fourcc = label;
        if (TssDelegate::personalizeComponent(context.ipswComponentPath, manifest, uploadPath,
                                              fourcc) != PrimitiveResult::Success) {
            Logger::error("  [Recovery] IMG4 personalize failed (ipsw img4 person)");
            return PrimitiveResult::Failed;
        }
        Logger::info("  [Recovery] personalized → " + uploadPath);
    }

    if (uploadPath.empty()) {
        Logger::info("  [Recovery] no upload path — set --recovery-upload or PURPLEPOIS0N_RECOVERY_UPLOAD");
        Logger::info("  [Recovery] or --ipsw-component + manifest for personalize-then-upload");
        return PrimitiveResult::Success;
    }

    if (!context.allowMutation) {
        Logger::info("  [Recovery] would upload " + label + ": " + uploadPath + " (probe only)");
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [Recovery] upload requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    try {
        Logger::info("  [Recovery] irecv_send_file " + label + ": " + uploadPath);
        if (!context.transport->sendFile(uploadPath)) {
            throw std::runtime_error("sendFile returned false");
        }
        Logger::info("  [Recovery] upload complete");

        const char* resetEnv = std::getenv("PURPLEPOIS0N_RECOVERY_RESET");
        if (resetEnv != nullptr && resetEnv[0] != '\0' && strcmp(resetEnv, "0") != 0) {
            if (context.transport->resetDevice()) {
                Logger::info("  [Recovery] irecv_reset complete");
            } else {
                Logger::warn("  [Recovery] irecv_reset failed or unsupported transport");
            }
        }
        const char* rebootEnv = std::getenv("PURPLEPOIS0N_RECOVERY_REBOOT");
        if (rebootEnv != nullptr && rebootEnv[0] != '\0' && strcmp(rebootEnv, "0") != 0) {
            if (context.transport->rebootDevice()) {
                Logger::info("  [Recovery] irecv_reboot issued");
            } else {
                Logger::warn("  [Recovery] irecv_reboot failed or unsupported transport");
            }
        }
        return PrimitiveResult::Success;
    } catch (const std::exception& e) {
        Logger::error(std::string("  [Recovery] upload failed: ") + e.what());
        return PrimitiveResult::Failed;
    }
}

} /* namespace primitives */
} /* namespace PP */
