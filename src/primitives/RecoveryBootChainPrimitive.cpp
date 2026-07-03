/*
 * RecoveryBootChainPrimitive.cpp
 */

#include "primitives/RecoveryBootChainPrimitive.h"
#include "primitives/TssDelegate.h"
#include "primitives/TssTypes.h"
#include "primitives/ITransport.h"
#include "RamdiskPackager.h"
#include "RamdiskWorkDir.h"
#include "EnvUtil.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* RecoveryBootChainPrimitive::name() const { return "recovery-boot-chain"; }

PrimitiveCategory RecoveryBootChainPrimitive::category() const {
    return PrimitiveCategory::Injection;
}

std::vector<PrimitiveOperation> RecoveryBootChainPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> RecoveryBootChainPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Recovery};
}

bool RecoveryBootChainPrimitive::canRun(const ExecutionContext& context) const {
    if (context.deviceState != DeviceState::Recovery) {
        return false;
    }
    if (!context.recoveryChain.empty()) {
        return true;
    }
    if (context.recoveryChainRun) {
        return true;
    }
    return PP::envFlagEnabled("PURPLEPOIS0N_RECOVERY_CHAIN");
}

namespace {

std::string resolveManifest(ExecutionContext& context) {
    if (!context.im4mManifestPath.empty()) {
        return context.im4mManifestPath;
    }
    const std::string fromEnv = PP::envOrEmpty("PURPLEPOIS0N_IM4M_MANIFEST");
    if (!fromEnv.empty()) {
        return fromEnv;
    }
    const std::string workDir = resolveRamdiskWorkDir(context.ramdiskWorkDir);
    if (!context.apticketPath.empty()) {
        const std::string manifest = workDir + "/im4m.im4m";
        if (TssDelegate::extractIm4mFromApticket(context.apticketPath, manifest) ==
            PrimitiveResult::Success) {
            context.im4mManifestPath = manifest;
            return manifest;
        }
    }
    if (!context.ipswPath.empty() &&
        TssDelegate::resolveSigningMode(context) == TssSigningMode::StockLive) {
        const std::string shsh = workDir + "/live.shsh";
        const std::string manifest = workDir + "/live.im4m";
        if (TssDelegate::fetchLiveIm4m(context, shsh, manifest) == PrimitiveResult::Success) {
            context.im4mManifestPath = manifest;
            return manifest;
        }
    }
    return std::string();
}

PrimitiveResult uploadComponent(ExecutionContext& context, const std::string& label,
                                const std::string& uploadPath) {
    if (context.transport == nullptr || !context.transport->isLive()) {
        Logger::warn("  [Recovery] chain requires RecoveryTransport");
        return PrimitiveResult::PrerequisitesMissing;
    }
    if (std::string(context.transport->transportName()) != "RecoveryTransport") {
        Logger::warn("  [Recovery] chain requires Recovery mode transport");
        return PrimitiveResult::TransportError;
    }
    if (!context.allowMutation) {
        Logger::info("  [Recovery] would upload " + label + ": " + uploadPath);
        return PrimitiveResult::Success;
    }
    if (!exploitPluginsEnabled()) {
        Logger::warn("  [Recovery] chain upload requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }
    Logger::info("  [Recovery] uploading " + label + ": " + uploadPath);
    if (!context.transport->sendFile(uploadPath)) {
        Logger::error("  [Recovery] upload failed: " + label);
        return PrimitiveResult::Failed;
    }
    if (PP::truthyEnv("PURPLEPOIS0N_RECOVERY_RESET", true)) {
        if (!context.transport->resetDevice()) {
            Logger::error("  [Recovery] irecv_reset failed after " + label);
            return PrimitiveResult::Failed;
        }
    }

    if (PP::truthyEnv("PURPLEPOIS0N_RECOVERY_PROMPT_CHECK", true)) {
        const std::string prompt = context.transport->getDeviceEnv("Prompt");
        if (!prompt.empty()) {
            Logger::info("  [Recovery] iBoot prompt after " + label + ": " + prompt);
            if (prompt.find("Error") != std::string::npos ||
                prompt.find("failed") != std::string::npos) {
                Logger::error("  [Recovery] upload may have failed — iBoot reported error prompt");
                return PrimitiveResult::Failed;
            }
        }
    }

    return PrimitiveResult::Success;
}

PrimitiveResult personalizeIfNeeded(const ExecutionContext& /*context*/, const std::string& manifest,
                                    const ExecutionContext::RecoveryChainComponent& component,
                                    const std::string& workDir, std::string* uploadPath) {
    if (!component.uploadPath.empty()) {
        *uploadPath = component.uploadPath;
        return PrimitiveResult::Success;
    }
    if (component.ipswComponentPath.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }
    if (manifest.empty()) {
        Logger::warn("  [Recovery] no IM4M for personalize — use --apticket or live TSS");
        return PrimitiveResult::PrerequisitesMissing;
    }
    *uploadPath = workDir + "/" + component.fourcc + ".img4";
    const std::string fourcc =
        (component.fourcc == "RestoreRamDisk" || component.fourcc == "rdsk")
            ? "RestoreRamDisk"
            : component.fourcc;
    return TssDelegate::personalizeComponent(component.ipswComponentPath, manifest, *uploadPath,
                                           fourcc);
}

} /* anonymous */

PrimitiveResult RecoveryBootChainPrimitive::execute(ExecutionContext& context) {
    Logger::info("  [Recovery] multi-stage boot chain (iBSS → iBEC → rdsk)");
    Logger::info("  [Recovery] Pongo/KPF/checkra1n ramdisk load — external delegate only");

    const std::string workDir = resolveRamdiskWorkDir(context.ramdiskWorkDir);
    ensureDirectory(workDir);

    std::vector<ExecutionContext::RecoveryChainComponent> chain = context.recoveryChain;
    if (chain.empty() && context.recoveryChainRun) {
        Logger::info("  [Recovery] default chain requires --ipsw component paths or recoveryChain");
    }

    const std::string manifest = resolveManifest(context);

    for (size_t i = 0; i < chain.size(); ++i) {
        ExecutionContext::RecoveryChainComponent& component = chain[i];
        if ((component.fourcc == "RestoreRamDisk" || component.fourcc == "rdsk") &&
            component.ipswComponentPath.empty() && !context.ipswPath.empty()) {
            RamdiskPackagerResult packResult;
            const std::string ident =
                context.ramdiskIdent.empty() ? std::string("Erase") : context.ramdiskIdent;
            if (packRamdiskFromIpsw(context.ramdiskBuild, context.ipswPath, ident,
                                    context.ramdiskOverlayPath, context.ramdiskStagedFiles, workDir,
                                    &packResult)) {
                component.ipswComponentPath = packResult.im4pPath;
            }
        }

        std::string uploadPath;
        const PrimitiveResult prep =
            personalizeIfNeeded(context, manifest, component, workDir, &uploadPath);
        if (prep != PrimitiveResult::Success) {
            if (!context.allowMutation) {
                Logger::info("  [Recovery] would personalize " + component.fourcc);
                continue;
            }
            return prep;
        }

        const PrimitiveResult up =
            uploadComponent(context, component.fourcc.empty() ? "component" : component.fourcc,
                            uploadPath);
        if (up != PrimitiveResult::Success) {
            return up;
        }
    }

    if (!context.allowMutation) {
        Logger::info("  [Recovery] boot command stage (probe): sendCommand go — not automated");
    } else {
        if (PP::envOrEmpty("PURPLEPOIS0N_RECOVERY_BOOT") == "0") {
            Logger::info("  [Recovery] PURPLEPOIS0N_RECOVERY_BOOT=0 — skipping go");
        } else if (context.transport != nullptr) {
            Logger::info("  [Recovery] sending iBoot boot command: go");
            if (!context.transport->sendCommand("go")) {
                Logger::error("  [Recovery] sendCommand go failed");
                return PrimitiveResult::Failed;
            }
        } else {
            Logger::error("  [Recovery] go requires RecoveryTransport");
            return PrimitiveResult::TransportError;
        }
    }

    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
