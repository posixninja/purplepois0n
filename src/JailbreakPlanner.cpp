/*
 * JailbreakPlanner.cpp
 */

#include "JailbreakPlanner.h"

#include "Checkm8.h"
#include "DeviceManager.h"
#include "EnvUtil.h"
#include "RamdiskDelivery.h"
#include "RamdiskWorkDir.h"
#include "Syringe.h"
#include "Usbliter8.h"
#include "pongo/PongoDevice.h"
#include "primitives/Gen6Types.h"
#include "primitives/PrimitiveTypes.h"
#include "primitives/RootlessDelegate.h"

#include <sstream>
#include <sys/stat.h>

namespace PP {
namespace {

std::string jsonEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (const char c : text) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

const char* deviceStateJson(DeviceState state) {
    switch (state) {
        case DeviceState::DFU:
            return "dfu";
        case DeviceState::Recovery:
            return "recovery";
        case DeviceState::Normal:
            return "normal";
        default:
            return "unknown";
    }
}

const char* eraJson(primitives::JailbreakGeneration era) {
    return primitives::jailbreakGenerationToString(era);
}

void addBlocker(JailbreakPlan* plan, const std::string& reason) {
    if (plan == nullptr) {
        return;
    }
    plan->blockers.push_back(reason);
    plan->canExecute = false;
}

primitives::JailbreakGeneration eraForProfile(const DeviceProfile& profile) {
    primitives::ExecutionContext ctx;
    ctx.deviceState = profile.state;
    ctx.cpid = profile.cpid;
    ctx.iosVersion = profile.iosVersion;
    ctx.productType = profile.productType;
    return primitives::detectJailbreakGeneration(ctx);
}

bool isRegularFile(const std::string& path) {
    struct stat st = {};
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string resolveDefaultIpswPath() {
    const std::string fromEnv = envOrEmpty("PURPLEPOIS0N_IPSW");
    if (isRegularFile(fromEnv)) {
        return fromEnv;
    }
    const char* candidates[] = {
        "/tmp/pp-kpf-fw/firmware.ipsw",
        "/tmp/firmware.ipsw",
    };
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        const std::string candidate = std::string(candidates[i]);
        if (isRegularFile(candidate)) {
            return candidate;
        }
    }
    return std::string();
}

void applyRamdiskDefaults(JailbreakPlan* plan) {
    if (plan == nullptr) {
        return;
    }
    const bool needsRamdisk = plan->useBootDelivery || plan->useRecoveryChain;
    if (!needsRamdisk) {
        return;
    }

    const std::string overlay = envOrEmpty("PURPLEPOIS0N_RAMDISK_OVERLAY");
    if (!overlay.empty()) {
        plan->options.ramdisk.overlayPath = overlay;
    }

    const std::string ramdiskEnv = envOrEmpty("PURPLEPOIS0N_RAMDISK");
    if (isRegularFile(ramdiskEnv)) {
        plan->options.ramdisk.artifactPath = ramdiskEnv;
        plan->ramdiskSource = "artifact";
        return;
    }

    if (!plan->options.ramdisk.artifactPath.empty()) {
        plan->ramdiskSource = "artifact";
        return;
    }

    const std::string ipsw = resolveDefaultIpswPath();
    if (!ipsw.empty()) {
        plan->options.ipswPath = ipsw;
        plan->ipswPath = ipsw;
        plan->ramdiskSource = "ipsw-pack";
        return;
    }

    addBlocker(plan, "ramdisk required — set PURPLEPOIS0N_IPSW or PURPLEPOIS0N_RAMDISK");
}

void applyRecoveryChainDefaults(JailbreakPlan* plan) {
    if (plan == nullptr || !plan->useRecoveryChain) {
        return;
    }
    if (plan->options.ipswPath.empty()) {
        return;
    }
    const std::string workDir = resolveRamdiskWorkDir(plan->options.ramdisk.workDir);
    if (!populateDefaultRecoveryChain(plan->options.ipswPath, workDir,
                                      &plan->options.recovery.chain)) {
        addBlocker(plan, "recovery chain: could not build stages from IPSW");
        return;
    }
    bool hasIbss = false;
    bool hasIbec = false;
    for (size_t i = 0; i < plan->options.recovery.chain.size(); ++i) {
        if (plan->options.recovery.chain[i].fourcc == "iBSS") {
            hasIbss = true;
        }
        if (plan->options.recovery.chain[i].fourcc == "iBEC") {
            hasIbec = true;
        }
    }
    if (!hasIbss) {
        addBlocker(plan, "recovery chain: iBSS not found in IPSW");
    }
    if (!hasIbec) {
        addBlocker(plan, "recovery chain: iBEC not found in IPSW");
    }
    const std::string apticket = envOrEmpty("PURPLEPOIS0N_APTICKET");
    const std::string im4m = envOrEmpty("PURPLEPOIS0N_IM4M_MANIFEST");
    if (apticket.empty() && im4m.empty()) {
        addBlocker(plan,
                   "TSS/apticket recommended — set PURPLEPOIS0N_APTICKET or "
                   "PURPLEPOIS0N_IM4M_MANIFEST for personalized recovery upload");
    }
}

} /* anonymous */

HostCapabilities probeHostCapabilities() {
    HostCapabilities host;
    host.pluginsEnabled = primitives::exploitPluginsEnabled();
    host.bootModulePath = resolveDefaultBootModulePath();
    host.bootModuleBuilt = !host.bootModulePath.empty();
    host.libusbLinked = pongoLibusbAvailable();
    return host;
}

bool buildDeviceProfile(DeviceManager& manager,
                        const std::string& targetUDID,
                        DeviceProfile* profile) {
    if (profile == nullptr) {
        return false;
    }

    profile->state = manager.detectDeviceState(targetUDID);
    if (profile->state == DeviceState::Unknown) {
        return false;
    }

    profile->udid = targetUDID;
    profile->checkm8Supported = profile->cpid != 0 && Checkm8::isSupportedCpid(profile->cpid);
    profile->usbliter8Supported = profile->cpid != 0 && Usbliter8::isSupportedCpid(profile->cpid);

    Syringe syringe;
    if (syringe.detectDevice(manager)) {
        uint32_t cpid = 0;
        uint64_t ecid = 0;
        if (syringe.queryCpid(cpid)) {
            profile->cpid = cpid;
        }
        if (syringe.queryEcid(ecid)) {
            profile->ecid = ecid;
        }
        profile->checkm8Supported = Checkm8::isSupportedCpid(profile->cpid);
        profile->usbliter8Supported = Usbliter8::isSupportedCpid(profile->cpid);
        profile->socName = Checkm8::cpidToSocName(profile->cpid);
    }

    if (profile->state == DeviceState::Normal) {
        try {
            auto device = manager.getMobileDevice(targetUDID);
            if (device) {
                profile->productType = device->getDeviceType();
                profile->iosVersion = device->getValue("", "ProductVersion");
                if (profile->udid.empty()) {
                    profile->udid = device->getUDID();
                }
            }
        } catch (const std::exception&) {
        }
    }

    if (profile->socName.empty() && profile->cpid != 0) {
        profile->socName = Checkm8::cpidToSocName(profile->cpid);
    }
    profile->era = eraForProfile(*profile);
    return true;
}

JailbreakPlan planJailbreak(const DeviceProfile& profile, const HostCapabilities& host) {
    JailbreakPlan plan;
    plan.options = Gen0Options();
    plan.bootModulePath = host.bootModulePath;
    plan.options.ramdisk.bootModulePath = host.bootModulePath;

    switch (profile.state) {
        case DeviceState::DFU:
            if (profile.checkm8Supported) {
                plan.strategyId = "dfu-checkm8-usb-loader";
                plan.summary = "checkm8 bootrom → USB loader boot (module + ramdisk)";
                plan.bootLane = BootDeliveryLane::UsbLoader;
                plan.useBootromExploit = true;
                plan.useBootDelivery = true;
                plan.options.ramdisk.deliveryLane = BootDeliveryLane::UsbLoader;
                plan.options.ramdisk.deliveryRun = true;
                plan.options.pongo.bootRun = true;
                plan.options.pongo.execute = true;
                plan.options.jailbreakExecute = true;
                if (!host.pluginsEnabled) {
                    addBlocker(&plan, "make plugins required for bootrom + boot delivery");
                }
                if (!host.bootModuleBuilt) {
                    addBlocker(&plan, "boot module missing — run: make kpf");
                }
                if (!host.libusbLinked) {
                    addBlocker(&plan, "libusb not linked — brew install libusb && rebuild");
                }
            } else if (profile.usbliter8Supported) {
                plan.strategyId = "dfu-usbliter8";
                plan.summary = "usbliter8 bootrom exploit (A12/A13/S4/S5)";
                plan.useBootromExploit = true;
                plan.options.jailbreakExecute = true;
                if (!host.pluginsEnabled) {
                    addBlocker(&plan, "make plugins required for usbliter8 path");
                }
            } else {
                plan.strategyId = "dfu-unsupported";
                plan.summary = "No in-tree bootrom exploit for this CPID";
                plan.canProbe = true;
                addBlocker(&plan, "unsupported CPID for checkm8/usbliter8");
            }
            break;

        case DeviceState::Recovery:
            plan.strategyId = "recovery-ramdisk-chain";
            plan.summary = "Recovery iBSS → iBEC → personalized ramdisk IM4P";
            plan.bootLane = BootDeliveryLane::Recovery;
            plan.useRecoveryChain = true;
            plan.options.recovery.chainRun = true;
            plan.options.ramdisk.deliveryLane = BootDeliveryLane::Recovery;
            if (!host.pluginsEnabled) {
                addBlocker(&plan, "make plugins required for recovery upload");
            }
            break;

        case DeviceState::Normal:
        default: {
            RamdiskConnectOptions connect;
            if (!profile.udid.empty()) {
                connect.udid = profile.udid;
            }
            connect.transport = RamdiskTransport::Ssh;
            connect.autoIproxy = true;
            primitives::ExecutionContext probeCtx;
            probeCtx.ramdiskConnect = connect;
            probeCtx.udid = profile.udid;
            probeCtx.iosVersion = profile.iosVersion;
            probeCtx.productType = profile.productType;
            probeCtx.allowMutation = false;
            const primitives::RootlessProbeResult rootless =
                primitives::RootlessDelegate::probeDevice(probeCtx);
            if (rootless.jbrootExists) {
                plan.strategyId = "normal-already-jailbroken";
                plan.summary = "Device has /var/jb — post-jailbreak store/bootstrap tasks";
                plan.alreadyJailbroken = true;
                plan.useExternalDelegate = true;
                plan.options.externalJailbreak = true;
                plan.options.externalSkipHelper = true;
                plan.canExecute = true;
                break;
            }

            if (profile.era == primitives::JailbreakGeneration::Gen6 && host.pluginsEnabled) {
                plan.strategyId = "normal-gen6-in-tree";
                plan.summary = "iOS 15+ in-tree Gen6 probe/execute chain";
                plan.useEraChain = true;
                plan.options.jailbreakExecute = true;
                plan.options.bypassIntegrity = true;
                plan.canExecute = true;
            } else {
                plan.strategyId = "normal-external-delegate";
                plan.summary = "Delegate to palera1n/checkra1n helper script";
                plan.useExternalDelegate = true;
                plan.options.externalJailbreak = true;
                plan.options.jailbreakExecute = true;
                if (!host.pluginsEnabled) {
                    addBlocker(&plan, "make plugins required for external jailbreak delegate");
                }
            }
            break;
        }
    }

    applyRamdiskDefaults(&plan);

    applyRecoveryChainDefaults(&plan);

    if (!plan.alreadyJailbroken && plan.useExternalDelegate &&
        plan.strategyId == "normal-external-delegate" &&
        envFlagEnabled("PURPLEPOIS0N_POST_JB_STORE")) {
        plan.options.postJbStoreSync = true;
    }

    if (plan.blockers.empty() &&
        (plan.useBootromExploit || plan.useBootDelivery || plan.useExternalDelegate ||
         plan.useEraChain || plan.useRecoveryChain || plan.alreadyJailbroken)) {
        plan.canExecute = true;
    }
    return plan;
}

void mergePlanIntoOptions(Gen0Options* options, const JailbreakPlan& plan) {
    if (options == nullptr) {
        return;
    }
    if (!plan.bootModulePath.empty()) {
        options->ramdisk.bootModulePath = plan.bootModulePath;
        options->pongo.kpfPath = plan.bootModulePath;
    }
    options->ramdisk.deliveryLane = plan.options.ramdisk.deliveryLane;
    options->ramdisk.deliveryRun = plan.options.ramdisk.deliveryRun;
    options->ramdisk.deliveryProbe = plan.options.ramdisk.deliveryProbe;
    if (plan.useBootDelivery || plan.useBootromExploit) {
        options->pongo.bootRun = plan.options.pongo.bootRun;
        options->pongo.execute = plan.options.pongo.execute;
        options->jailbreakExecute = plan.options.jailbreakExecute;
    }
    if (plan.useRecoveryChain) {
        options->recovery.chainRun = plan.options.recovery.chainRun;
        options->recovery.execute = plan.options.recovery.execute;
        options->recovery.chain = plan.options.recovery.chain;
        if (options->jailbreakExecute) {
            options->recovery.execute = true;
        }
    }
    if (plan.useExternalDelegate) {
        options->externalJailbreak = plan.options.externalJailbreak;
        options->externalSkipHelper = plan.options.externalSkipHelper;
        options->postJbStoreSync = plan.options.postJbStoreSync;
        options->ramdisk.connect.transport = RamdiskTransport::Ssh;
        options->ramdisk.connect.autoIproxy = true;
    }
    if (plan.useEraChain) {
        options->jailbreakExecute = plan.options.jailbreakExecute;
        options->bypassIntegrity = plan.options.bypassIntegrity;
    }
    if (!plan.options.ipswPath.empty()) {
        options->ipswPath = plan.options.ipswPath;
    }
    if (!plan.options.ramdisk.artifactPath.empty()) {
        options->ramdisk.artifactPath = plan.options.ramdisk.artifactPath;
        options->pongo.ramdiskDmgPath = plan.options.ramdisk.artifactPath;
    }
    if (!plan.options.ramdisk.overlayPath.empty()) {
        options->ramdisk.overlayPath = plan.options.ramdisk.overlayPath;
    }
}

std::string jailbreakPlanToJson(const DeviceProfile& profile, const JailbreakPlan& plan) {
    std::ostringstream json;
    json << "{";
    json << "\"device\":{";
    json << "\"state\":\"" << deviceStateJson(profile.state) << "\"";
    if (profile.cpid != 0) {
        json << ",\"cpid\":\"0x" << std::hex << profile.cpid << std::dec << "\"";
    }
    if (profile.ecid != 0) {
        json << ",\"ecid\":\"" << profile.ecid << "\"";
    }
    if (!profile.socName.empty()) {
        json << ",\"soc\":\"" << jsonEscape(profile.socName) << "\"";
    }
    if (!profile.productType.empty()) {
        json << ",\"productType\":\"" << jsonEscape(profile.productType) << "\"";
    }
    if (!profile.iosVersion.empty()) {
        json << ",\"iosVersion\":\"" << jsonEscape(profile.iosVersion) << "\"";
    }
    if (!profile.udid.empty()) {
        json << ",\"udid\":\"" << jsonEscape(profile.udid) << "\"";
    }
    json << ",\"era\":\"" << eraJson(profile.era) << "\"";
    json << ",\"checkm8\":" << (profile.checkm8Supported ? "true" : "false");
    json << ",\"usbliter8\":" << (profile.usbliter8Supported ? "true" : "false");
    json << "},";
    json << "\"plan\":{";
    json << "\"strategy\":\"" << jsonEscape(plan.strategyId) << "\"";
    json << ",\"summary\":\"" << jsonEscape(plan.summary) << "\"";
    json << ",\"bootLane\":\"" << bootDeliveryLaneLabel(plan.bootLane) << "\"";
    json << ",\"canProbe\":" << (plan.canProbe ? "true" : "false");
    json << ",\"canExecute\":" << (plan.canExecute ? "true" : "false");
    json << ",\"useBootromExploit\":" << (plan.useBootromExploit ? "true" : "false");
    json << ",\"useBootDelivery\":" << (plan.useBootDelivery ? "true" : "false");
    json << ",\"useExternalDelegate\":" << (plan.useExternalDelegate ? "true" : "false");
    json << ",\"useEraChain\":" << (plan.useEraChain ? "true" : "false");
    json << ",\"useRecoveryChain\":" << (plan.useRecoveryChain ? "true" : "false");
    json << ",\"alreadyJailbroken\":" << (plan.alreadyJailbroken ? "true" : "false");
    if (!plan.bootModulePath.empty()) {
        json << ",\"bootModule\":\"" << jsonEscape(plan.bootModulePath) << "\"";
    }
    if (!plan.ipswPath.empty()) {
        json << ",\"ipsw\":\"" << jsonEscape(plan.ipswPath) << "\"";
    }
    if (!plan.ramdiskSource.empty()) {
        json << ",\"ramdiskSource\":\"" << jsonEscape(plan.ramdiskSource) << "\"";
    }
    if (plan.useRecoveryChain && !plan.options.recovery.chain.empty()) {
        json << ",\"recoveryChain\":[";
        for (size_t i = 0; i < plan.options.recovery.chain.size(); ++i) {
            if (i > 0) {
                json << ',';
            }
            const auto& stage = plan.options.recovery.chain[i];
            json << "{\"fourcc\":\"" << jsonEscape(stage.fourcc) << "\"";
            if (!stage.ipswComponentPath.empty()) {
                json << ",\"component\":\"" << jsonEscape(stage.ipswComponentPath) << "\"";
            }
            json << "}";
        }
        json << "]";
    }
    json << ",\"blockers\":[";
    for (size_t i = 0; i < plan.blockers.size(); ++i) {
        if (i > 0) {
            json << ',';
        }
        json << "\"" << jsonEscape(plan.blockers[i]) << "\"";
    }
    json << "]";
    json << "}";
    json << "}";
    return json.str();
}

} /* namespace PP */
