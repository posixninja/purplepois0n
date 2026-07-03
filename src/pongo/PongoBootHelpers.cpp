/*
 * PongoBootHelpers.cpp
 */

#include "pongo/PongoBootHelpers.h"
#include "RamdiskPackager.h"
#include "RamdiskWorkDir.h"
#include "EnvUtil.h"
#include "Logger.h"

#include <fstream>
#include <sys/stat.h>

namespace PP {

namespace {

bool fileExists(const std::string& path) {
    struct stat st;
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string defaultBuiltKpfPath() {
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

} /* anonymous */

std::string resolvePongoKpfPath(const primitives::ExecutionContext& context) {
    if (!context.pongoKpfPath.empty()) {
        return context.pongoKpfPath;
    }
    const std::string fromEnv = envOrEmpty("PURPLEPOIS0N_KPF");
    if (!fromEnv.empty()) {
        return fromEnv;
    }
    return defaultBuiltKpfPath();
}

std::string resolvePongoXargs(const primitives::ExecutionContext& context) {
    if (!context.pongoXargsLine.empty()) {
        return context.pongoXargsLine;
    }
    const std::string fromEnv = envOrEmpty("PURPLEPOIS0N_PONGO_XARGS");
    if (!fromEnv.empty()) {
        return fromEnv;
    }
    return "xargs serial=3 rootdev=md0";
}

bool readFileBytes(const std::string& path, std::vector<uint8_t>* out) {
    if (out == nullptr || path.empty()) {
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        Logger::error("  [Pongo] cannot read: " + path);
        return false;
    }
    in.seekg(0, std::ios::end);
    const std::streamsize size = in.tellg();
    if (size <= 0) {
        Logger::error("  [Pongo] empty file: " + path);
        return false;
    }
    in.seekg(0, std::ios::beg);
    out->resize(static_cast<size_t>(size));
    if (!in.read(reinterpret_cast<char*>(out->data()), size)) {
        Logger::error("  [Pongo] read failed: " + path);
        return false;
    }
    return true;
}

bool resolvePongoRamdiskDmg(primitives::ExecutionContext& context, std::string* dmgPath) {
    if (dmgPath == nullptr) {
        return false;
    }
    if (!context.pongoRamdiskDmgPath.empty()) {
        *dmgPath = context.pongoRamdiskDmgPath;
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
            context.pongoRamdiskDmgPath = packResult.dmgPath;
            *dmgPath = packResult.dmgPath;
            return true;
        }
        return false;
    }
    Logger::warn("  [Pongo] ramdisk DMG required — --pongo-ramdisk, --build-ramdisk, or --ipsw");
    return false;
}

} /* namespace PP */
