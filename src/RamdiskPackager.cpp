/*
 * RamdiskPackager.cpp
 */

#include "RamdiskPackager.h"
#include "RamdiskBuilder.h"
#include "RamdiskStager.h"
#include "RamdiskWorkDir.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <vector>

namespace PP {

namespace {

bool runSimple(const std::vector<std::string>& argv) {
    return ToolRunner::run(argv).exitCode == 0;
}

bool readFileBytes(const std::string& path, std::vector<uint8_t>* out) {
    if (out == nullptr) {
        return false;
    }
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    out->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool writeFileBytes(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(&data[0]), static_cast<std::streamsize>(data.size()));
    }
    return out.good();
}

bool extractHfsTree(const std::string& ipswBin, const std::string& dmgPath,
                    const std::string& outDir) {
    ensureDirectory(outDir);
    std::vector<std::string> argv;
    argv.push_back(ipswBin);
    argv.push_back("disk");
    argv.push_back("hfs");
    argv.push_back(dmgPath);
    argv.push_back("-p");
    argv.push_back(".*");
    argv.push_back("-o");
    argv.push_back(outDir);
    Logger::info("  [Ramdisk] extracting stock HFS+ tree via ipsw...");
    return runSimple(argv);
}

RamdiskOptions mergedRamdiskOptions(const RamdiskOptions& options, const std::string& stockDmgPath) {
    RamdiskOptions merged = options;
    struct stat st;
    if (stat(stockDmgPath.c_str(), &st) == 0 && st.st_size > 0) {
        const uint64_t stockBytes = static_cast<uint64_t>(st.st_size);
        if (stockBytes > merged.sizeBytes) {
            merged.sizeBytes = stockBytes;
        }
        merged.sizeBytes += stockBytes / 10;
    }
    return merged;
}

bool buildMergedRamdisk(const RamdiskOptions& options, const std::string& stockExtractDir,
                        const std::string& overlayDir,
                        const std::vector<RamdiskStageEntry>& stagedFiles,
                        const std::string& dmgPath) {
    RamdiskBuilder builder(options);
    if (!stockExtractDir.empty()) {
        builder.addOverlayDirectory(stockExtractDir);
    }
    if (!overlayDir.empty()) {
        builder.addOverlayDirectory(overlayDir);
    }
    if (!stagedFiles.empty() && !stageHostFiles(&builder, stagedFiles)) {
        return false;
    }
    if (!builder.buildToFile(dmgPath)) {
        Logger::error("  [Ramdisk] merged build failed");
        return false;
    }
    Logger::info("  [Ramdisk] merged stock + overlay → " + dmgPath);
    return true;
}

} /* anonymous */

bool buildRamdiskDmg(const RamdiskOptions& options, const std::string& overlayDir,
                     const std::vector<RamdiskStageEntry>& stagedFiles,
                     const std::string& dmgPath) {
    RamdiskBuilder builder(options);
    if (!overlayDir.empty()) {
        builder.addOverlayDirectory(overlayDir);
    }
    if (!stagedFiles.empty() && !stageHostFiles(&builder, stagedFiles)) {
        return false;
    }
    if (!builder.buildToFile(dmgPath)) {
        Logger::error("  [Ramdisk] build failed");
        return false;
    }
    Logger::info("  [Ramdisk] wrote HFS+ → " + dmgPath);
    return true;
}

bool wrapDmgAsIm4p(const std::string& dmgPath, const std::string& im4pPath) {
    const std::string ipsw = ToolRunner::findIpswExecutable();
    if (ipsw.empty()) {
        Logger::error("  [Ramdisk] ipsw not found — make external-ipsw");
        return false;
    }
    std::vector<std::string> argv;
    argv.push_back(ipsw);
    argv.push_back("img4");
    argv.push_back("im4p");
    argv.push_back("create");
    argv.push_back("--type");
    argv.push_back("rdsk");
    argv.push_back("--compress");
    argv.push_back("lzss");
    argv.push_back("--output");
    argv.push_back(im4pPath);
    argv.push_back(dmgPath);
    Logger::info("  [Ramdisk] wrapping IM4P rdsk...");
    return runSimple(argv);
}

bool packRamdiskFromIpsw(const RamdiskOptions& options, const std::string& ipswPath,
                         const std::string& ident, const std::string& overlayDir,
                         const std::vector<RamdiskStageEntry>& stagedFiles,
                         const std::string& workDir, RamdiskPackagerResult* result) {
    if (result == nullptr || ipswPath.empty()) {
        return false;
    }
    if (!ensureDirectory(workDir)) {
        Logger::error("  [Ramdisk] work dir unavailable: " + workDir);
        return false;
    }

    const std::string ipsw = ToolRunner::findIpswExecutable();
    if (ipsw.empty()) {
        Logger::error("  [Ramdisk] ipsw not found");
        return false;
    }

    const std::string extractDir = workDir + "/ipsw-extract";
    ensureDirectory(extractDir);

    std::vector<std::string> extractArgv;
    extractArgv.push_back(ipsw);
    extractArgv.push_back("extract");
    extractArgv.push_back("--dmg");
    extractArgv.push_back("rdisk");
    extractArgv.push_back("-o");
    extractArgv.push_back(extractDir);
    if (!ident.empty()) {
        extractArgv.push_back("--ident");
        extractArgv.push_back(ident);
    }
    extractArgv.push_back(ipswPath);
    Logger::info("  [Ramdisk] extracting RestoreRamDisk from IPSW...");
    if (!runSimple(extractArgv)) {
        Logger::error("  [Ramdisk] ipsw extract rdisk failed");
        return false;
    }

    std::string im4pBlob;
    {
        std::vector<std::string> findArgv;
        findArgv.push_back("/usr/bin/find");
        findArgv.push_back(extractDir);
        findArgv.push_back("-name");
        findArgv.push_back("*.dmg");
        findArgv.push_back("-maxdepth");
        findArgv.push_back("2");
        const CommandResult findResult = ToolRunner::run(findArgv);
        if (findResult.exitCode != 0 || findResult.stdoutText.empty()) {
            Logger::error("  [Ramdisk] no .dmg in extracted IPSW tree");
            return false;
        }
        im4pBlob = findResult.stdoutText;
        while (!im4pBlob.empty() && (im4pBlob.back() == '\n' || im4pBlob.back() == '\r')) {
            im4pBlob.pop_back();
        }
    }

    const std::string stockDmg = workDir + "/stock.dmg";
    std::vector<std::string> unwrapArgv;
    unwrapArgv.push_back(ipsw);
    unwrapArgv.push_back("img4");
    unwrapArgv.push_back("im4p");
    unwrapArgv.push_back("extract");
    unwrapArgv.push_back("--output");
    unwrapArgv.push_back(stockDmg);
    unwrapArgv.push_back(im4pBlob);
    Logger::info("  [Ramdisk] unwrapping IM4P → HFS+...");
    if (!runSimple(unwrapArgv)) {
        Logger::error("  [Ramdisk] im4p extract failed");
        return false;
    }

    result->dmgPath = workDir + "/modified.dmg";
    if (!overlayDir.empty() || !stagedFiles.empty()) {
        const std::string stockTree = workDir + "/stock-hfs";
        if (!extractHfsTree(ipsw, stockDmg, stockTree)) {
            Logger::error("  [Ramdisk] stock HFS+ extract failed");
            return false;
        }
        const RamdiskOptions mergedOptions = mergedRamdiskOptions(options, stockDmg);
        if (!buildMergedRamdisk(mergedOptions, stockTree, overlayDir, stagedFiles,
                                result->dmgPath)) {
            return false;
        }
    } else {
        std::vector<uint8_t> bytes;
        if (!readFileBytes(stockDmg, &bytes)) {
            return false;
        }
        if (!writeFileBytes(result->dmgPath, bytes)) {
            return false;
        }
    }

    result->im4pPath = workDir + "/ramdisk.im4p";
    if (!wrapDmgAsIm4p(result->dmgPath, result->im4pPath)) {
        return false;
    }

    Logger::info("  [Ramdisk] IM4P → " + result->im4pPath);
    return true;
}

bool locateIpswIm4pComponent(const std::string& ipswPath, const std::string& workDir,
                             const std::string& pattern, std::string* outPath) {
    if (outPath == nullptr || ipswPath.empty() || pattern.empty()) {
        return false;
    }
    if (!ensureDirectory(workDir)) {
        return false;
    }
    const std::string ipsw = ToolRunner::findIpswExecutable();
    if (ipsw.empty()) {
        return false;
    }
    const std::string extractDir = workDir + "/component-extract";
    ensureDirectory(extractDir);

    std::vector<std::string> extractArgv;
    extractArgv.push_back(ipsw);
    extractArgv.push_back("extract");
    extractArgv.push_back("--pattern");
    extractArgv.push_back(pattern);
    extractArgv.push_back("-o");
    extractArgv.push_back(extractDir);
    extractArgv.push_back(ipswPath);
    if (!runSimple(extractArgv)) {
        return false;
    }

    std::vector<std::string> findArgv;
    findArgv.push_back("/usr/bin/find");
    findArgv.push_back(extractDir);
    findArgv.push_back("-name");
    findArgv.push_back("*.im4p");
    findArgv.push_back("-maxdepth");
    findArgv.push_back("4");
    const CommandResult findResult = ToolRunner::run(findArgv);
    if (findResult.exitCode != 0 || findResult.stdoutText.empty()) {
        return false;
    }
    std::string path = findResult.stdoutText;
    while (!path.empty() && (path.back() == '\n' || path.back() == '\r')) {
        path.pop_back();
    }
    if (path.empty()) {
        return false;
    }
    *outPath = path;
    return true;
}

} /* namespace PP */
