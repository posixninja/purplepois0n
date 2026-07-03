/*
 * MedicineDelegate.cpp
 */

#include "primitives/MedicineDelegate.h"
#include "primitives/RootlessLayout.h"
#include "DeviceManager.h"
#include "MobileDevice.h"
#include "RamdiskClient.h"
#include "EnvUtil.h"
#include "Logger.h"

#include <sys/stat.h>
#include <unistd.h>

namespace PP {
namespace primitives {

namespace {

bool pathIsDirectory(const std::string& path) {
    struct stat st = {};
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool sshConfigured(const ExecutionContext& context) {
    const RamdiskConnectOptions& connect = context.ramdiskConnect;
    if (!connect.udid.empty()) {
        return true;
    }
    if (connect.transport == RamdiskTransport::Ssh) {
        return true;
    }
    return PP::envFlagEnabled("PURPLEPOIS0N_NORMAL_SSH");
}

MedicineCureStep makeStep(MedicineCureId id, const std::string& summary,
                          const std::string& legacyRef, bool requiresSsh, bool autoApplicable) {
    MedicineCureStep step;
    step.id = id;
    step.name = medicineCureIdToString(id);
    step.summary = summary;
    step.legacyReference = legacyRef;
    step.requiresSsh = requiresSsh;
    step.autoApplicable = autoApplicable;
    return step;
}

bool cureSelected(const std::vector<MedicineCureId>& selected, MedicineCureId id) {
    for (size_t i = 0; i < selected.size(); ++i) {
        if (selected[i] == id) {
            return true;
        }
    }
    return false;
}

std::string shellQuote(const std::string& text) {
    std::string out = "'";
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\'') {
            out += "'\\''";
        } else {
            out += text[i];
        }
    }
    out += "'";
    return out;
}

std::string afc2ShellScript() {
    return std::string(
        "set -e; "
        "PB=/usr/libexec/PlistBuddy; "
        "for PL in /System/Library/Lockdown/Services.plist "
        "/var/root/Library/Lockdown/Services.plist; do "
        "  if [ -f \"$PL\" ] && [ -w \"$PL\" ]; then "
        "    $PB -c 'Add :com.apple.afc2 dict' \"$PL\" 2>/dev/null || true; "
        "    $PB -c 'Add :com.apple.afc2:AllowUnactivatedService bool true' \"$PL\" 2>/dev/null || "
        "      $PB -c 'Set :com.apple.afc2:AllowUnactivatedService true' \"$PL\"; "
        "    $PB -c 'Add :com.apple.afc2:Label string com.apple.afc2' \"$PL\" 2>/dev/null || true; "
        "    $PB -c 'Add :com.apple.afc2:ProgramArguments array' \"$PL\" 2>/dev/null || true; "
        "    $PB -c 'Add :com.apple.afc2:ProgramArguments:0 string /usr/libexec/afcd' \"$PL\" "
        "      2>/dev/null || true; "
        "    $PB -c 'Add :com.apple.afc2:ProgramArguments:1 string --lockdown' \"$PL\" "
        "      2>/dev/null || true; "
        "    $PB -c 'Add :com.apple.afc2:ProgramArguments:2 string -d' \"$PL\" 2>/dev/null || true; "
        "    $PB -c 'Add :com.apple.afc2:ProgramArguments:3 string /' \"$PL\" 2>/dev/null || true; "
        "    echo \"medicine: afc2 patched $PL\"; "
        "  fi; "
        "done");
}

std::string capableShellScript(const std::string& platform, const std::string& capability) {
    const std::string plPath =
        "/System/Library/CoreServices/SpringBoard.app/" + platform + ".plist";
    return std::string("set -e; PB=/usr/libexec/PlistBuddy; PL=") + shellQuote(plPath) +
           "; CAP=" + shellQuote(capability) +
           "; if [ -f \"$PL\" ] && [ -w \"$PL\" ]; then "
           "$PB -c \"Delete :capabilities:$CAP\" \"$PL\" 2>/dev/null || true; "
           "echo \"medicine: removed capability $CAP from $PL\"; "
           "else echo \"medicine: capable skip (missing or read-only $PL)\"; fi";
}

std::string sachetShellScript(const std::string& appPath) {
    return std::string(
               "echo 'medicine: sachet requires on-device Foundation tools (legacy iOS 4–6); "
               "register app manually or install loader deb'; "
               "test -d ") +
           shellQuote(appPath) + " && echo 'medicine: app bundle present at " + appPath + "'";
}

PrimitiveResult applyViaSsh(const ExecutionContext& context, const std::string& script,
                            const char* label) {
    RamdiskClient client(context.ramdiskConnect);
    std::string message;
    if (!client.probe(&message)) {
        Logger::error(std::string("  [Medicine] SSH probe failed (") + label + "): " + message);
        return PrimitiveResult::TransportError;
    }
    const RamdiskCommandResult result = client.exec(script);
    if (!result.stdoutText.empty()) {
        Logger::info("  [Medicine] " + result.stdoutText);
    }
    if (!result.stderrText.empty()) {
        Logger::warn("  [Medicine] " + result.stderrText);
    }
    if (result.exitCode != 0) {
        Logger::error(std::string("  [Medicine] ") + label + " failed (exit " +
                      std::to_string(result.exitCode) + ")");
        return PrimitiveResult::Failed;
    }
    return PrimitiveResult::Success;
}

} /* anonymous */

std::string MedicineDelegate::resolveMedicineRoot() {
    const std::string fromEnv = PP::envOrEmpty("PURPLEPOIS0N_MEDICINE_ROOT");
    if (!fromEnv.empty() && pathIsDirectory(fromEnv)) {
        return fromEnv;
    }
    const std::string legacy = "legacy/Chronic-Dev/medicine";
    if (pathIsDirectory(legacy)) {
        return legacy;
    }
    return fromEnv.empty() ? legacy : fromEnv;
}

bool MedicineDelegate::queryActivation(const std::string& udid, MedicineProfile* profile) {
    if (profile == nullptr || udid.empty()) {
        return false;
    }
    try {
        MobileDevice device(udid);
        profile->activationState = device.getValue("", "ActivationState");
        profile->productType = device.getValue("", "ProductType");
        profile->iosVersion = device.getValue("", "ProductVersion");
        profile->activated =
            profile->activationState == "Activated" || profile->activationState == "FactoryActivated";
        return !profile->activationState.empty();
    } catch (const std::exception& e) {
        Logger::warn(std::string("  [Medicine] lockdown query failed: ") + e.what());
        return false;
    }
}

MedicineProfile MedicineDelegate::buildPlan(const ExecutionContext& context) {
    MedicineProfile profile;
    profile.jbroot = RootlessLayout::resolveJbroot();
    profile.medicineRoot = resolveMedicineRoot();

    if (!context.udid.empty()) {
        (void)queryActivation(context.udid, &profile);
    }
    if (profile.productType.empty() && !context.productType.empty()) {
        profile.productType = context.productType;
    }
    if (profile.iosVersion.empty() && !context.iosVersion.empty()) {
        profile.iosVersion = context.iosVersion;
    }

    const std::vector<MedicineCureId> selected = parseMedicineCureList(context.medicineCures);
    const bool needsHacktivation = !profile.activated && !profile.activationState.empty();
    const bool hasMirror = pathIsDirectory(profile.medicineRoot);

    if (cureSelected(selected, MedicineCureId::ActivationProbe) || profile.activationState.empty()) {
        MedicineCureStep step = makeStep(
            MedicineCureId::ActivationProbe,
            profile.activationState.empty() ? "Query ActivationState via lockdown"
                                            : "ActivationState=" + profile.activationState,
            "lockdownd ActivationState", false, true);
        step.selected = true;
        profile.plan.push_back(step);
    }

    if (cureSelected(selected, MedicineCureId::Afc2Unactivated)) {
        MedicineCureStep step = makeStep(
            MedicineCureId::Afc2Unactivated,
            needsHacktivation
                ? "Add com.apple.afc2 with AllowUnactivatedService (USB FS on SIM-less setup)"
                : "AFC2 unactivated service (useful when device skips activation UI)",
            "medicine/afc2add/afc2add.mm", true, needsHacktivation || sshConfigured(context));
        step.selected = true;
        profile.plan.push_back(step);
    }

    if (cureSelected(selected, MedicineCureId::CapableStrip)) {
        const std::string cap = context.medicineCapability.empty() ? "wifi" : context.medicineCapability;
        const std::string platform =
            context.medicinePlatform.empty() ? profile.productType : context.medicinePlatform;
        MedicineCureStep step = makeStep(
            MedicineCureId::CapableStrip,
            "Remove SpringBoard capability '" + cap + "' from platform " + platform,
            "medicine/capable/capable.mm", true, !platform.empty() && sshConfigured(context));
        step.selected = true;
        profile.plan.push_back(step);
    }

    if (cureSelected(selected, MedicineCureId::SachetRegister)) {
        MedicineCureStep step = makeStep(
            MedicineCureId::SachetRegister,
            context.medicineAppPath.empty()
                ? "Register system app in mobileinstallation cache (needs --medicine-app)"
                : "Register app at " + context.medicineAppPath,
            "medicine/sachet/sachet.mm", true, !context.medicineAppPath.empty());
        step.selected = true;
        profile.plan.push_back(step);
    }

    if (cureSelected(selected, MedicineCureId::LoaderHint)) {
        MedicineCureStep step = makeStep(
            MedicineCureId::LoaderHint,
            hasMirror ? "Study medicine/loader (Cydia bootstrap UI) at " + profile.medicineRoot
                      : "Clone Chronic-Dev/medicine (legacy/clone-chronic-dev.sh) for loader sources",
            "medicine/loader", false, hasMirror);
        step.selected = true;
        profile.plan.push_back(step);
    }

    return profile;
}

void MedicineDelegate::logMedicinePlan(const MedicineProfile& profile) {
    Logger::info("  [Medicine] post-install cures (GreenPois0n medicine lineage)");
    if (!profile.activationState.empty()) {
        Logger::info("  [Medicine]   ActivationState: " + profile.activationState +
                     (profile.activated ? " (activated)" : " (needs hacktivation cures)"));
    }
    if (!profile.productType.empty()) {
        Logger::info("  [Medicine]   ProductType: " + profile.productType);
    }
    if (!profile.jbroot.empty()) {
        Logger::info("  [Medicine]   jbroot hint: " + profile.jbroot);
    }
    Logger::info("  [Medicine]   mirror: " + profile.medicineRoot +
                 (pathIsDirectory(profile.medicineRoot) ? "" : " (missing — run legacy/clone-chronic-dev.sh)"));
    for (size_t i = 0; i < profile.plan.size(); ++i) {
        const MedicineCureStep& step = profile.plan[i];
        std::string line = "  [Medicine]   • " + step.name + ": " + step.summary;
        if (step.requiresSsh) {
            line += step.autoApplicable ? " [ssh-ready]" : " [ssh-required]";
        }
        Logger::info(line);
        if (!step.legacyReference.empty()) {
            Logger::info("  [Medicine]       ref: " + step.legacyReference);
        }
    }
}

PrimitiveResult MedicineDelegate::runMedicine(const ExecutionContext& context, bool allowMutation) {
    const MedicineProfile profile = buildPlan(context);
    logMedicinePlan(profile);

    if (!allowMutation) {
        Logger::info("  [Medicine] probe-only — use --medicine-apply or --post-jb-pipeline --medicine");
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [Medicine] apply requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    if (!sshConfigured(context)) {
        Logger::error("  [Medicine] apply requires jailbroken SSH (--normal-ssh or ramdisk connect)");
        return PrimitiveResult::PrerequisitesMissing;
    }

    PrimitiveResult overall = PrimitiveResult::Success;
    const std::string platform =
        context.medicinePlatform.empty() ? profile.productType : context.medicinePlatform;
    const std::string capability =
        context.medicineCapability.empty() ? "wifi" : context.medicineCapability;

    for (size_t i = 0; i < profile.plan.size(); ++i) {
        const MedicineCureStep& step = profile.plan[i];
        if (!step.selected || !step.autoApplicable) {
            continue;
        }
        switch (step.id) {
            case MedicineCureId::ActivationProbe:
                break;
            case MedicineCureId::Afc2Unactivated: {
                const PrimitiveResult r = applyViaSsh(context, afc2ShellScript(), "afc2");
                if (r != PrimitiveResult::Success) {
                    overall = r;
                }
                break;
            }
            case MedicineCureId::CapableStrip: {
                if (platform.empty()) {
                    Logger::warn("  [Medicine] capable skip — set --medicine-platform");
                    break;
                }
                const PrimitiveResult r =
                    applyViaSsh(context, capableShellScript(platform, capability), "capable");
                if (r != PrimitiveResult::Success) {
                    overall = r;
                }
                break;
            }
            case MedicineCureId::SachetRegister: {
                if (context.medicineAppPath.empty()) {
                    break;
                }
                const PrimitiveResult r =
                    applyViaSsh(context, sachetShellScript(context.medicineAppPath), "sachet");
                if (r != PrimitiveResult::Success) {
                    overall = r;
                }
                break;
            }
            case MedicineCureId::LoaderHint:
                Logger::info("  [Medicine] loader: build/install org.chronic-dev.loader from mirror");
                break;
        }
    }

    if (overall == PrimitiveResult::Success) {
        Logger::info("  [Medicine] cures applied — respring or reboot may be required");
    }
    return overall;
}

} /* namespace primitives */
} /* namespace PP */
