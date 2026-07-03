/*
 * DpkgStore.cpp
 */

#include "store/DpkgStore.h"
#include "primitives/RootlessLayout.h"
#include "EnvUtil.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <cstdio>

namespace PP {
namespace store {

namespace {

const char* kSuite = "purplepois0n";
const char* kComponent = "main";
const char* kDefaultArch = "iphoneos-arm64";
const char* kDeviceStoreRel = "var/mobile/purplepois0n-store";
const char* kSourcesListRel = "etc/apt/sources.list.d/purplepois0n.list";

bool ensureDirRecursive(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    struct stat st = {};
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    const size_t slash = path.find_last_of('/');
    if (slash != std::string::npos && slash > 0) {
        if (!ensureDirRecursive(path.substr(0, slash))) {
            return false;
        }
    }
    return mkdir(path.c_str(), 0755) == 0 || (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

std::string trim(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && (text[start] == ' ' || text[start] == '\t')) {
        ++start;
    }
    size_t end = text.size();
    while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r')) {
        --end;
    }
    return text.substr(start, end - start);
}

std::string joinPath(const std::string& base, const std::string& leaf) {
    if (base.empty()) {
        return leaf;
    }
    if (leaf.empty()) {
        return base;
    }
    if (base.back() == '/') {
        return base + leaf;
    }
    return base + "/" + leaf;
}

bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string packagesDirForArch(const std::string& storeRoot, const std::string& arch) {
    return joinPath(storeRoot, std::string("dists/") + kSuite + "/" + kComponent + "/binary-" + arch);
}

std::string poolMainDir(const std::string& storeRoot) {
    return joinPath(storeRoot, "pool/main");
}

std::string readFileToString(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) {
        return std::string();
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

bool writeStringToFile(const std::string& path, const std::string& body) {
    if (!ensureDirRecursive(path.substr(0, path.find_last_of('/')))) {
        return false;
    }
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out << body;
    return out.good();
}

uint64_t fileSizeBytes(const std::string& path) {
    struct stat st = {};
    if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        return 0;
    }
    return static_cast<uint64_t>(st.st_size);
}

std::string sha256HexFile(const std::string& path) {
    const std::string shasum = ToolRunner::findExecutable("shasum");
    if (!shasum.empty()) {
        std::vector<std::string> argv;
        argv.push_back(shasum);
        argv.push_back("-a");
        argv.push_back("256");
        argv.push_back(path);
        const CommandResult run = ToolRunner::run(argv);
        if (run.exitCode == 0 && !run.stdoutText.empty()) {
            const size_t space = run.stdoutText.find(' ');
            return trim(space == std::string::npos ? run.stdoutText : run.stdoutText.substr(0, space));
        }
    }
    const std::string openssl = ToolRunner::findExecutable("openssl");
    if (!openssl.empty()) {
        std::vector<std::string> argv;
        argv.push_back(openssl);
        argv.push_back("dgst");
        argv.push_back("-sha256");
        argv.push_back("-r");
        argv.push_back(path);
        const CommandResult run = ToolRunner::run(argv);
        if (run.exitCode == 0 && !run.stdoutText.empty()) {
            const size_t space = run.stdoutText.find(' ');
            return trim(space == std::string::npos ? run.stdoutText : run.stdoutText.substr(0, space));
        }
    }
    return std::string();
}

void parseControlFields(const std::string& controlText, DebControl* control) {
    std::istringstream lines(controlText);
    std::string line;
    std::string currentKey;
    std::string currentValue;
    auto flushField = [&]() {
        if (currentKey.empty()) {
            return;
        }
        const std::string value = trim(currentValue);
        if (currentKey == "Package") {
            control->package = value;
        } else if (currentKey == "Version") {
            control->version = value;
        } else if (currentKey == "Architecture") {
            control->architecture = value;
        } else if (currentKey == "Maintainer") {
            control->maintainer = value;
        } else if (currentKey == "Description") {
            if (control->description.empty()) {
                control->description = value;
            } else {
                control->description += " " + value;
            }
        }
        currentKey.clear();
        currentValue.clear();
    };

    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            flushField();
            continue;
        }
        if (line[0] == ' ' || line[0] == '\t') {
            if (!currentKey.empty()) {
                if (!currentValue.empty()) {
                    currentValue += " ";
                }
                currentValue += trim(line);
            }
            continue;
        }
        flushField();
        const size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        currentKey = trim(line.substr(0, colon));
        currentValue = trim(line.substr(colon + 1));
    }
    flushField();
}

bool extractControlViaDpkgDeb(const std::string& debPath, std::string* controlText) {
    const std::string dpkgDeb = ToolRunner::findExecutable("dpkg-deb");
    if (dpkgDeb.empty()) {
        return false;
    }
    const std::string tmpDir = "/tmp/pp-deb-ctl-" + std::to_string(getpid());
    std::vector<std::string> extractArgv;
    extractArgv.push_back(dpkgDeb);
    extractArgv.push_back("-e");
    extractArgv.push_back(debPath);
    extractArgv.push_back(tmpDir);
    const CommandResult extractRun = ToolRunner::run(extractArgv);
    if (extractRun.exitCode != 0) {
        return false;
    }
    const std::string controlPath = joinPath(tmpDir, "control");
    *controlText = readFileToString(controlPath);
    std::vector<std::string> rmArgv;
    rmArgv.push_back("/bin/rm");
    rmArgv.push_back("-rf");
    rmArgv.push_back(tmpDir);
    ToolRunner::run(rmArgv);
    return !controlText->empty();
}

bool extractControlViaArTar(const std::string& debPath, std::string* controlText) {
    const std::string ar = ToolRunner::findExecutable("ar");
    const std::string tar = ToolRunner::findExecutable("tar");
    if (ar.empty() || tar.empty()) {
        return false;
    }

    const char* members[] = {"control.tar.zst", "control.tar.xz", "control.tar.gz", "control.tar"};
    for (size_t i = 0; i < sizeof(members) / sizeof(members[0]); ++i) {
        std::vector<std::string> arArgv;
        arArgv.push_back(ar);
        arArgv.push_back("p");
        arArgv.push_back(debPath);
        arArgv.push_back(members[i]);
        const CommandResult arRun = ToolRunner::run(arArgv);
        if (arRun.exitCode != 0 || arRun.stdoutText.empty()) {
            continue;
        }

        const std::string tmpTar = "/tmp/pp-deb-control-" + std::to_string(getpid()) + ".tar";
        std::ofstream tmpOut(tmpTar.c_str(), std::ios::binary | std::ios::trunc);
        if (!tmpOut) {
            continue;
        }
        tmpOut << arRun.stdoutText;
        tmpOut.close();

        std::vector<std::string> tarArgv;
        tarArgv.push_back(tar);
        tarArgv.push_back("-xOf");
        tarArgv.push_back(tmpTar);
        tarArgv.push_back("./control");
        const CommandResult tarRun = ToolRunner::run(tarArgv);
        unlink(tmpTar.c_str());
        if (tarRun.exitCode == 0 && !tarRun.stdoutText.empty()) {
            *controlText = tarRun.stdoutText;
            return true;
        }
    }
    return false;
}

void collectDebFilesRecursive(const std::string& dir, std::vector<std::string>* out) {
    DIR* handle = opendir(dir.c_str());
    if (handle == nullptr) {
        return;
    }
    struct dirent* entry = nullptr;
    while ((entry = readdir(handle)) != nullptr) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        const std::string path = joinPath(dir, entry->d_name);
        struct stat st = {};
        if (stat(path.c_str(), &st) != 0) {
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            collectDebFilesRecursive(path, out);
        } else if (S_ISREG(st.st_mode) && endsWith(path, ".deb")) {
            out->push_back(path);
        }
    }
    closedir(handle);
}

std::string poolRelativeFilename(const std::string& storeRoot, const std::string& debPath) {
    const std::string pool = poolMainDir(storeRoot);
    if (debPath.size() >= pool.size() && debPath.compare(0, pool.size(), pool) == 0) {
        std::string rel = debPath.substr(pool.size());
        if (!rel.empty() && rel[0] == '/') {
            rel = rel.substr(1);
        }
        return joinPath("pool/main", rel);
    }
    const size_t slash = debPath.find_last_of('/');
    const std::string base = slash == std::string::npos ? debPath : debPath.substr(slash + 1);
    return joinPath("pool/main", base);
}

std::string formatPackagesEntry(const DebControl& control) {
    std::ostringstream entry;
    entry << "Package: " << control.package << "\n";
    entry << "Version: " << control.version << "\n";
    entry << "Architecture: " << control.architecture << "\n";
    if (!control.maintainer.empty()) {
        entry << "Maintainer: " << control.maintainer << "\n";
    }
    if (!control.description.empty()) {
        entry << "Description: " << control.description << "\n";
    }
    entry << "Filename: " << control.filename << "\n";
    entry << "Size: " << control.size << "\n";
    if (!control.sha256.empty()) {
        entry << "SHA256: " << control.sha256 << "\n";
    }
    entry << "\n";
    return entry.str();
}

std::string relativeFromStoreRoot(const std::string& storeRoot, const std::string& path) {
    if (storeRoot.empty()) {
        return path;
    }
    if (path.size() >= storeRoot.size() && path.compare(0, storeRoot.size(), storeRoot) == 0) {
        std::string rel = path.substr(storeRoot.size());
        if (!rel.empty() && rel[0] == '/') {
            rel = rel.substr(1);
        }
        return rel;
    }
    return path;
}

std::string rfc822DateUtc() {
    std::time_t now = std::time(nullptr);
    std::tm tmUtc = {};
#if defined(_WIN32)
    gmtime_s(&tmUtc, &now);
#else
    gmtime_r(&now, &tmUtc);
#endif
    char buf[64] = {};
    if (std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S +0000", &tmUtc) == 0) {
        return "Thu, 01 Jan 1970 00:00:00 +0000";
    }
    return std::string(buf);
}

bool compressFileViaTool(const std::string& inputPath, const std::string& outputPath,
                         const char* toolName, const char* flag) {
    const std::string tool = ToolRunner::findExecutable(toolName);
    if (tool.empty()) {
        return false;
    }
    std::vector<std::string> argv;
    argv.push_back(tool);
    if (flag != nullptr && flag[0] != '\0') {
        argv.push_back(flag);
    }
    argv.push_back("-c");
    argv.push_back(inputPath);
    const CommandResult run = ToolRunner::run(argv);
    if (run.exitCode != 0 || run.stdoutText.empty()) {
        return false;
    }
    return writeStringToFile(outputPath, run.stdoutText);
}

bool copyDirRecursive(const std::string& srcDir, const std::string& destDir) {
    struct stat st = {};
    if (stat(srcDir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
        return false;
    }
    const size_t slash = destDir.find_last_of('/');
    if (slash != std::string::npos) {
        if (!ensureDirRecursive(destDir.substr(0, slash))) {
            return false;
        }
    }
    const std::string cp = ToolRunner::findExecutable("cp");
    if (cp.empty()) {
        return false;
    }
    std::vector<std::string> argv;
    argv.push_back(cp);
    argv.push_back("-R");
    argv.push_back(srcDir);
    argv.push_back(destDir);
    const CommandResult run = ToolRunner::run(argv);
    return run.exitCode == 0;
}

bool copyTree(const std::string& srcRoot, const std::string& destRoot) {
    return copyDirRecursive(srcRoot, destRoot);
}

} /* anonymous */

std::string resolveStoreRoot() {
    const std::string env = envOrEmpty("PURPLEPOIS0N_STORE_ROOT");
    if (!env.empty()) {
        return env;
    }
    return "store";
}

std::string deviceStoreRelativePath() { return kDeviceStoreRel; }

std::string deviceStorePath(const std::string& jbroot) {
    return primitives::RootlessLayout::jbPath("/" + std::string(kDeviceStoreRel), jbroot);
}

std::string deviceSourcesListPath(const std::string& jbroot) {
    return primitives::RootlessLayout::jbPath("/" + std::string(kSourcesListRel), jbroot);
}

bool initRepository(const std::string& storeRoot, std::string* error) {
    const std::string root = storeRoot.empty() ? resolveStoreRoot() : storeRoot;
    if (!ensureDirRecursive(poolMainDir(root))) {
        if (error != nullptr) {
            *error = "failed to create pool/main under " + root;
        }
        return false;
    }
    if (!ensureDirRecursive(packagesDirForArch(root, kDefaultArch))) {
        if (error != nullptr) {
            *error = "failed to create dists layout under " + root;
        }
        return false;
    }
    Logger::info("  [Store] initialized repo at " + root);
    return true;
}

bool readDebControl(const std::string& debPath, DebControl* control, std::string* error) {
    if (control == nullptr) {
        if (error != nullptr) {
            *error = "null control output";
        }
        return false;
    }
    struct stat st = {};
    if (stat(debPath.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        if (error != nullptr) {
            *error = "deb not found: " + debPath;
        }
        return false;
    }

    std::string controlText;
    if (!extractControlViaDpkgDeb(debPath, &controlText) &&
        !extractControlViaArTar(debPath, &controlText)) {
        if (error != nullptr) {
            *error = "could not read DEBIAN/control from " + debPath;
        }
        return false;
    }

    parseControlFields(controlText, control);
    if (control->package.empty() || control->version.empty()) {
        if (error != nullptr) {
            *error = "missing Package/Version in control for " + debPath;
        }
        return false;
    }
    if (control->architecture.empty()) {
        control->architecture = kDefaultArch;
    }
    control->size = fileSizeBytes(debPath);
    control->sha256 = sha256HexFile(debPath);
    return true;
}

std::vector<std::string> listDebPaths(const std::string& storeRoot) {
    const std::string root = storeRoot.empty() ? resolveStoreRoot() : storeRoot;
    std::vector<std::string> paths;
    collectDebFilesRecursive(poolMainDir(root), &paths);
    std::sort(paths.begin(), paths.end());
    return paths;
}

StoreBuildResult buildRepository(const std::string& storeRoot) {
    StoreBuildResult result;
    result.storeRoot = storeRoot.empty() ? resolveStoreRoot() : storeRoot;

    std::string initError;
    if (!initRepository(result.storeRoot, &initError)) {
        result.error = initError;
        return result;
    }

    const std::vector<std::string> debPaths = listDebPaths(result.storeRoot);
    std::ostringstream packagesBody;

    for (size_t i = 0; i < debPaths.size(); ++i) {
        DebControl control;
        std::string readError;
        if (!readDebControl(debPaths[i], &control, &readError)) {
            Logger::warn("  [Store] skip " + debPaths[i] + ": " + readError);
            continue;
        }
        control.filename = poolRelativeFilename(result.storeRoot, debPaths[i]);
        packagesBody << formatPackagesEntry(control);
        ++result.packageCount;
        Logger::info("  [Store] indexed " + control.package + " " + control.version);
    }

    const std::string packagesPath = joinPath(packagesDirForArch(result.storeRoot, kDefaultArch), "Packages");
    const std::string packagesFlatPath = joinPath(result.storeRoot, "Packages");
    const std::string body = packagesBody.str();
    if (!writeStringToFile(packagesPath, body)) {
        result.error = "failed to write " + packagesPath;
        return result;
    }
    if (!writeStringToFile(packagesFlatPath, body)) {
        result.error = "failed to write " + packagesFlatPath;
        return result;
    }
    result.packagesPath = packagesPath;

    result.packagesGzPath = packagesPath + ".gz";
    if (!compressFileViaTool(packagesPath, result.packagesGzPath, "gzip", "-n")) {
        Logger::warn("  [Store] Packages.gz not written (gzip missing or failed)");
        result.packagesGzPath.clear();
    }

    result.packagesBz2Path = packagesPath + ".bz2";
    if (!compressFileViaTool(packagesPath, result.packagesBz2Path, "bzip2", "")) {
        Logger::warn("  [Store] Packages.bz2 not written (bzip2 missing or failed)");
        result.packagesBz2Path.clear();
    }

    const std::string packagesRel = relativeFromStoreRoot(result.storeRoot, packagesPath);
    const std::string packagesGzRel =
        result.packagesGzPath.empty() ? std::string() : relativeFromStoreRoot(result.storeRoot, result.packagesGzPath);
    const std::string packagesBz2Rel = result.packagesBz2Path.empty()
                                             ? std::string()
                                             : relativeFromStoreRoot(result.storeRoot, result.packagesBz2Path);

    std::ostringstream releaseBody;
    releaseBody << "Origin: purplepois0n\n";
    releaseBody << "Label: purplepois0n\n";
    releaseBody << "Suite: " << kSuite << "\n";
    releaseBody << "Codename: " << kSuite << "\n";
    releaseBody << "Architectures: " << kDefaultArch << " arm64\n";
    releaseBody << "Components: " << kComponent << "\n";
    releaseBody << "Date: " << rfc822DateUtc() << "\n";
    releaseBody << "SHA256:\n";
    const std::string packagesSha = sha256HexFile(packagesPath);
    if (!packagesSha.empty()) {
        releaseBody << " " << packagesSha << " " << packagesRel << "\n";
    }
    if (!result.packagesGzPath.empty()) {
        const std::string gzSha = sha256HexFile(result.packagesGzPath);
        if (!gzSha.empty()) {
            releaseBody << " " << gzSha << " " << packagesGzRel << "\n";
        }
    }
    if (!result.packagesBz2Path.empty()) {
        const std::string bz2Sha = sha256HexFile(result.packagesBz2Path);
        if (!bz2Sha.empty()) {
            releaseBody << " " << bz2Sha << " " << packagesBz2Rel << "\n";
        }
    }
    result.releasePath = joinPath(result.storeRoot, "dists/" + std::string(kSuite) + "/Release");
    if (!writeStringToFile(result.releasePath, releaseBody.str())) {
        result.error = "failed to write Release";
        return result;
    }

    result.success = true;
    Logger::info("  [Store] built " + std::to_string(result.packageCount) + " package(s)");
    return result;
}

const char* repositorySuite() { return kSuite; }

StorePublishResult publishRepository(const std::string& storeRoot, const std::string& publishRoot) {
    StorePublishResult result;
    const std::string root = storeRoot.empty() ? resolveStoreRoot() : storeRoot;
    const std::string dest = publishRoot.empty() ? (root + "-publish") : publishRoot;
    result.publishRoot = dest;

    const StoreBuildResult built = buildRepository(root);
    if (!built.success) {
        result.error = built.error.empty() ? "store build failed" : built.error;
        return result;
    }

    if (!ensureDirRecursive(dest)) {
        result.error = "failed to create publish dir " + dest;
        return result;
    }

    const std::vector<std::string> copyRoots = {
        joinPath(root, "pool"),
        joinPath(root, "dists"),
    };
    for (size_t i = 0; i < copyRoots.size(); ++i) {
        struct stat st = {};
        if (stat(copyRoots[i].c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
            continue;
        }
        const std::string rel = relativeFromStoreRoot(root, copyRoots[i]);
        if (!copyTree(copyRoots[i], joinPath(dest, rel))) {
            result.error = "failed to copy " + copyRoots[i];
            return result;
        }
    }

    const std::string nginxHint = joinPath(dest, "README-hosting.txt");
    const std::string hintBody =
        "purplepois0n apt repo — serve this directory over HTTPS.\n\n"
        "nginx example:\n"
        "  location / {\n"
        "    root /path/to/this/dir;\n"
        "    autoindex off;\n"
        "  }\n\n"
        "Device sources.list.d entry:\n"
        "  deb [trusted=yes] https://YOUR_HOST/ " +
        std::string(kSuite) + " " + std::string(kComponent) + "\n";
    if (!writeStringToFile(nginxHint, hintBody)) {
        Logger::warn("  [Store] could not write " + nginxHint);
    }

    result.success = true;
    Logger::info("  [Store] published repo to " + dest);
    return result;
}

std::string aptSourceLine(const std::string& jbroot) {
    const std::string storePath = deviceStorePath(jbroot);
    return std::string("deb [trusted=yes] file://") + storePath + " " + kSuite + " " + kComponent;
}

bool addDebToPool(const std::string& storeRoot, const std::string& debPath, std::string* error) {
    DebControl control;
    if (!readDebControl(debPath, &control, error)) {
        return false;
    }
    const std::string root = storeRoot.empty() ? resolveStoreRoot() : storeRoot;
    std::string initError;
    if (!initRepository(root, &initError)) {
        if (error != nullptr) {
            *error = initError;
        }
        return false;
    }

    const std::string first = control.package.empty() ? "misc" : std::string(1, control.package[0]);
    const std::string destDir = joinPath(poolMainDir(root), joinPath(first, control.package));
    if (!ensureDirRecursive(destDir)) {
        if (error != nullptr) {
            *error = "failed to create pool dir " + destDir;
        }
        return false;
    }

    const size_t slash = debPath.find_last_of('/');
    const std::string baseName = slash == std::string::npos ? debPath : debPath.substr(slash + 1);
    const std::string destPath = joinPath(destDir, baseName);

    std::ifstream in(debPath.c_str(), std::ios::binary);
    std::ofstream out(destPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!in || !out) {
        if (error != nullptr) {
            *error = "copy failed for " + debPath;
        }
        return false;
    }
    out << in.rdbuf();
    Logger::info("  [Store] added " + control.package + " → " + destPath);
    return out.good();
}

} /* namespace store */
} /* namespace PP */
