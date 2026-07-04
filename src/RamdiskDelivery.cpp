/*
 * RamdiskDelivery.cpp
 *
 * Ramdisk artifacts and boot modules are resolved independently.
 */

#include "RamdiskDelivery.h"
#include "RamdiskPackager.h"
#include "RamdiskWorkDir.h"
#include "EnvUtil.h"
#include "Logger.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sys/stat.h>

namespace PP {
namespace {

bool fileExists(const std::string& path) {
    struct stat st = {};
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string lowerExt(const std::string& path) {
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return std::string();
    }
    std::string ext = path.substr(dot + 1);
    for (size_t i = 0; i < ext.size(); ++i) {
        ext[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));
    }
    return ext;
}

std::string envBootModule() {
    const std::string module = envOrEmpty("PURPLEPOIS0N_BOOT_MODULE");
    if (!module.empty()) {
        return module;
    }
    return envOrEmpty("PURPLEPOIS0N_KPF");
}

std::string envBootArgs() {
    const std::string args = envOrEmpty("PURPLEPOIS0N_BOOT_ARGS");
    if (!args.empty()) {
        return args;
    }
    return envOrEmpty("PURPLEPOIS0N_PONGO_XARGS");
}

std::string defaultBuiltBootModulePath() {
    const std::string fromEnv = envOrEmpty("PURPLEPOIS0N_KPF_BUILD");
    if (!fromEnv.empty()) {
        return fromEnv;
    }
    const char* roots[] = {
        "legacy/kpf-purple/build/purplepois0n-kpf-pongo",
        "legacy/modern-era/PongoOS/build/purplepois0n-kpf-pongo",
    };
    for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); ++i) {
        const std::string candidate = std::string(roots[i]);
        if (fileExists(candidate)) {
            return candidate;
        }
    }
    return std::string();
}

BootDeliveryLane laneFromEnv() {
    const std::string lane = envOrEmpty("PURPLEPOIS0N_BOOT_LANE");
    if (lane.empty()) {
        return BootDeliveryLane::Auto;
    }
    return bootDeliveryLaneFromString(lane);
}

bool legacyPongoBootFlags(const primitives::ExecutionContext& context) {
    return context.pongoBootRun || context.pongoProbeRun ||
           PP::envFlagEnabled("PURPLEPOIS0N_PONGO_BOOT") ||
           PP::envFlagEnabled("PURPLEPOIS0N_PONGO_PROBE");
}

} /* anonymous */

RamdiskArtifactFormat ramdiskArtifactFormatFromString(const std::string& text) {
    std::string lower = text;
    for (size_t i = 0; i < lower.size(); ++i) {
        lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
    }
    if (lower == "raw" || lower == "dmg" || lower == "raw-dmg" || lower == "hfs") {
        return RamdiskArtifactFormat::RawDmg;
    }
    if (lower == "im4p" || lower == "rdsk") {
        return RamdiskArtifactFormat::Im4p;
    }
    return RamdiskArtifactFormat::Auto;
}

const char* ramdiskArtifactFormatLabel(const RamdiskArtifactFormat format) {
    switch (format) {
        case RamdiskArtifactFormat::RawDmg:
            return "raw-dmg";
        case RamdiskArtifactFormat::Im4p:
            return "im4p";
        case RamdiskArtifactFormat::Auto:
        default:
            return "auto";
    }
}

BootDeliveryLane bootDeliveryLaneFromString(const std::string& text) {
    std::string lower = text;
    for (size_t i = 0; i < lower.size(); ++i) {
        lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
    }
    if (lower == "recovery" || lower == "rec") {
        return BootDeliveryLane::Recovery;
    }
    if (lower == "usb-loader" || lower == "usb" || lower == "pongo" || lower == "loader") {
        return BootDeliveryLane::UsbLoader;
    }
    if (lower == "post-exploit" || lower == "postexploit" || lower == "anthrax" ||
        lower == "inject") {
        return BootDeliveryLane::PostExploit;
    }
    if (lower == "live" || lower == "live-agent" || lower == "agent") {
        return BootDeliveryLane::LiveAgent;
    }
    if (lower == "host" || lower == "build" || lower == "host-build") {
        return BootDeliveryLane::HostBuild;
    }
    return BootDeliveryLane::Auto;
}

const char* bootDeliveryLaneLabel(const BootDeliveryLane lane) {
    switch (lane) {
        case BootDeliveryLane::Recovery:
            return "recovery";
        case BootDeliveryLane::UsbLoader:
            return "usb-loader";
        case BootDeliveryLane::PostExploit:
            return "post-exploit";
        case BootDeliveryLane::LiveAgent:
            return "live-agent";
        case BootDeliveryLane::HostBuild:
            return "host-build";
        case BootDeliveryLane::Auto:
        default:
            return "auto";
    }
}

RamdiskArtifactFormat inferArtifactFormat(const std::string& path, const RamdiskArtifactFormat hint) {
    if (hint != RamdiskArtifactFormat::Auto) {
        return hint;
    }
    const std::string ext = lowerExt(path);
    if (ext == "im4p") {
        return RamdiskArtifactFormat::Im4p;
    }
    if (ext == "dmg") {
        return RamdiskArtifactFormat::RawDmg;
    }
    return RamdiskArtifactFormat::Auto;
}

std::string resolveDefaultBootModulePath() {
    const std::string fromEnv = envBootModule();
    if (!fromEnv.empty()) {
        return fromEnv;
    }
    return defaultBuiltBootModulePath();
}

std::string resolveBootModulePath(const primitives::ExecutionContext& context) {
    if (!context.bootModulePath.empty()) {
        return context.bootModulePath;
    }
    if (!context.pongoKpfPath.empty()) {
        return context.pongoKpfPath;
    }
    return resolveDefaultBootModulePath();
}

std::string resolveBootArgsLine(const primitives::ExecutionContext& context) {
    if (!context.bootArgsLine.empty()) {
        return context.bootArgsLine;
    }
    if (!context.pongoXargsLine.empty()) {
        return context.pongoXargsLine;
    }
    return envBootArgs();
}

bool bootDeliveryRequested(const primitives::ExecutionContext& context) {
    if (context.bootDeliveryRun || context.bootDeliveryProbe) {
        return true;
    }
    if (context.recoveryChainRun) {
        return true;
    }
    if (legacyPongoBootFlags(context)) {
        return true;
    }
    if (!context.ramdiskExecCommand.empty() || !context.ramdiskConnect.udid.empty()) {
        return true;
    }
    return PP::envFlagEnabled("PURPLEPOIS0N_BOOT_DELIVERY") ||
           PP::envFlagEnabled("PURPLEPOIS0N_POST_EXPLOIT_RAMDISK");
}

BootDeliverySpec resolveBootDelivery(const primitives::ExecutionContext& context) {
    BootDeliverySpec spec;
    spec.artifactPath = context.ramdiskArtifactPath;
    spec.format = inferArtifactFormat(spec.artifactPath, context.ramdiskArtifactFormat);
    spec.modulePath = resolveBootModulePath(context);
    spec.bootArgsLine = resolveBootArgsLine(context);

    if (context.bootDeliveryLane != BootDeliveryLane::Auto) {
        spec.lane = context.bootDeliveryLane;
    } else {
        const BootDeliveryLane fromEnv = laneFromEnv();
        if (fromEnv != BootDeliveryLane::Auto) {
            spec.lane = fromEnv;
        } else if (context.bootDeliveryRun || context.bootDeliveryProbe ||
                   legacyPongoBootFlags(context)) {
            spec.lane = BootDeliveryLane::UsbLoader;
        } else if (context.recoveryChainRun) {
            spec.lane = BootDeliveryLane::Recovery;
        } else if (!context.ramdiskExecCommand.empty() || !context.ramdiskConnect.udid.empty()) {
            spec.lane = BootDeliveryLane::LiveAgent;
        } else if (PP::envFlagEnabled("PURPLEPOIS0N_POST_EXPLOIT_RAMDISK")) {
            spec.lane = BootDeliveryLane::PostExploit;
        }
    }

    if (spec.lane == BootDeliveryLane::Auto && !spec.artifactPath.empty()) {
        if (spec.format == RamdiskArtifactFormat::Im4p) {
            spec.lane = BootDeliveryLane::Recovery;
        } else if (spec.format == RamdiskArtifactFormat::RawDmg) {
            spec.lane = BootDeliveryLane::UsbLoader;
        }
    }

    return spec;
}

bool resolveRamdiskArtifactPath(primitives::ExecutionContext& context, std::string* outPath) {
    if (outPath == nullptr) {
        return false;
    }

    if (!context.ramdiskArtifactPath.empty()) {
        *outPath = context.ramdiskArtifactPath;
        return true;
    }

    if (!context.ipswPath.empty()) {
        const std::string workDir = resolveRamdiskWorkDir(context.ramdiskWorkDir);
        RamdiskPackagerResult packResult;
        const std::string ident =
            context.ramdiskIdent.empty() ? std::string("Erase") : context.ramdiskIdent;
        if (packRamdiskFromIpsw(context.ramdiskBuild, context.ipswPath, ident,
                                context.ramdiskOverlayPath, context.ramdiskStagedFiles, workDir,
                                &packResult)) {
            context.ramdiskArtifactPath = packResult.dmgPath;
            context.pongoRamdiskDmgPath = packResult.dmgPath;
            *outPath = packResult.dmgPath;
            return true;
        }
        return false;
    }

    Logger::warn("  [Boot] ramdisk artifact required — --ramdisk PATH, --build-ramdisk, or --ipsw");
    return false;
}

bool readBootArtifactBytes(const std::string& path, std::vector<uint8_t>* out) {
    if (out == nullptr || path.empty()) {
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        Logger::error("  [Boot] cannot read: " + path);
        return false;
    }
    in.seekg(0, std::ios::end);
    const std::streamsize size = in.tellg();
    if (size <= 0) {
        Logger::error("  [Boot] empty file: " + path);
        return false;
    }
    in.seekg(0, std::ios::beg);
    out->resize(static_cast<size_t>(size));
    if (!in.read(reinterpret_cast<char*>(out->data()), size)) {
        Logger::error("  [Boot] read failed: " + path);
        return false;
    }
    return true;
}

} /* namespace PP */
