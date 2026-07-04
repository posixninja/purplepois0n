/*
 * DpkgStoreSync.cpp
 */

#include "store/DpkgStoreSync.h"
#include "primitives/RootlessLayout.h"
#include "RamdiskClient.h"
#include "Logger.h"

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <cstring>

namespace PP {
namespace store {

namespace {

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

bool ensureRemoteDir(RamdiskClient& client, const std::string& remoteDir) {
    const RamdiskCommandResult result = client.exec("mkdir -p " + shellQuote(remoteDir));
    return result.exitCode == 0;
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

bool uploadStoreFile(RamdiskClient& client, const std::string& localPath,
                     const std::string& storeRoot, const std::string& deviceStorePath) {
    const std::string rel = relativeFromStoreRoot(storeRoot, localPath);
    const std::string remotePath = joinPath(deviceStorePath, rel);
    const size_t slash = remotePath.find_last_of('/');
    if (slash != std::string::npos) {
        if (!ensureRemoteDir(client, remotePath.substr(0, slash))) {
            return false;
        }
    }
    Logger::info("  [Store] push " + rel);
    return client.uploadFile(localPath, remotePath);
}

void collectFilesRecursive(const std::string& dir, std::vector<std::string>* out) {
    if (out == nullptr) {
        return;
    }
    DIR* dp = opendir(dir.c_str());
    if (dp == nullptr) {
        return;
    }
    struct dirent* entry = nullptr;
    while ((entry = readdir(dp)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        const std::string path = joinPath(dir, entry->d_name);
        struct stat st = {};
        if (stat(path.c_str(), &st) != 0) {
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            collectFilesRecursive(path, out);
        } else if (S_ISREG(st.st_mode)) {
            out->push_back(path);
        }
    }
    closedir(dp);
}

std::vector<std::string> listRepoTreeFiles(const std::string& storeRoot) {
    std::vector<std::string> files;
    const std::vector<std::string> roots = {
        joinPath(storeRoot, "pool"),
        joinPath(storeRoot, "dists"),
    };
    for (size_t i = 0; i < roots.size(); ++i) {
        struct stat st = {};
        if (stat(roots[i].c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            collectFilesRecursive(roots[i], &files);
        }
    }
    return files;
}

bool writeSourcesList(RamdiskClient& client, const std::string& jbroot,
                      const std::string& sourcesListPath, StoreSyncMode mode) {
    std::string sourceLine;
    if (mode == StoreSyncMode::SourcesOnly) {
        const std::string repoUrl = resolveRepoUrl();
        if (repoUrl.find("YOUR_HOST") == std::string::npos) {
            sourceLine = aptHttpsSourceLine();
        } else {
            sourceLine = aptSourceLine(jbroot);
        }
    } else {
        sourceLine = aptSourceLine(jbroot);
    }

    const std::string tmpSources = "/tmp/pp-purplepois0n-sources-" + std::to_string(getpid()) + ".list";
    {
        std::ofstream out(tmpSources.c_str(), std::ios::trunc);
        if (!out) {
            return false;
        }
        out << sourceLine << "\n";
    }
    const bool ok = client.uploadFile(tmpSources, sourcesListPath);
    unlink(tmpSources.c_str());
    return ok;
}

void runAptUpdate(RamdiskClient& client, const std::string& jbroot) {
    const std::string aptBin = jbroot + "/usr/bin/apt";
    const std::string updateCmd = "test -x " + shellQuote(aptBin) + " && " + shellQuote(aptBin) +
                                  " update 2>/dev/null || true";
    client.exec(updateCmd);
}

} /* anonymous */

StoreSyncMode parseStoreSyncMode(const std::string& mode) {
    if (mode == "https") {
        return StoreSyncMode::Https;
    }
    if (mode == "sources-only" || mode == "sources_only" || mode == "sourcesonly") {
        return StoreSyncMode::SourcesOnly;
    }
    return StoreSyncMode::File;
}

const char* storeSyncModeLabel(StoreSyncMode mode) {
    switch (mode) {
        case StoreSyncMode::Https:
            return "https";
        case StoreSyncMode::SourcesOnly:
            return "sources-only";
        case StoreSyncMode::File:
        default:
            return "file";
    }
}

StoreSyncResult syncStoreToDevice(const RamdiskConnectOptions& connect, const std::string& storeRoot,
                                  bool allowMutation, StoreSyncMode mode) {
    StoreSyncResult result;
    const std::string root = storeRoot.empty() ? resolveStoreRoot() : storeRoot;
    const std::string jbroot = primitives::RootlessLayout::resolveJbroot();
    result.deviceStorePath = deviceStorePath(jbroot);
    result.sourcesListPath = deviceSourcesListPath(jbroot);

    StoreBuildResult built = buildRepository(root);
    if (!built.success) {
        result.error = built.error.empty() ? "store build failed" : built.error;
        return result;
    }

    Logger::info(std::string("  [Store] sync mode: ") + storeSyncModeLabel(mode));

    if (!allowMutation) {
        Logger::info("  [Store] dry-run sync → " + result.deviceStorePath);
        result.success = true;
        return result;
    }

    if (mode == StoreSyncMode::Https) {
        Logger::info("  [Store] https mode — skip device mirror; install purplepois0n-sources or purplepois0n-zebra on device");
        result.success = true;
        return result;
    }

    RamdiskClient client(connect);
    std::string probeMsg;
    if (!client.probe(&probeMsg)) {
        result.error = probeMsg.empty() ? "SSH probe failed" : probeMsg;
        return result;
    }

    const bool uploadMirror = mode == StoreSyncMode::File;
    if (uploadMirror) {
        if (!ensureRemoteDir(client, result.deviceStorePath)) {
            result.error = "failed to create device store dir";
            return result;
        }

        const std::vector<std::string> repoFiles = listRepoTreeFiles(root);
        for (size_t i = 0; i < repoFiles.size(); ++i) {
            if (!uploadStoreFile(client, repoFiles[i], root, result.deviceStorePath)) {
                result.error = "failed to upload " + relativeFromStoreRoot(root, repoFiles[i]);
                return result;
            }
        }
    }

    const std::string aptDir = primitives::RootlessLayout::jbPath("/etc/apt/sources.list.d", jbroot);
    if (!ensureRemoteDir(client, aptDir)) {
        result.error = "failed to create apt sources dir";
        return result;
    }

    if (!writeSourcesList(client, jbroot, result.sourcesListPath, mode)) {
        result.error = "failed to install apt source list";
        return result;
    }

    runAptUpdate(client, jbroot);

    if (uploadMirror) {
        const std::vector<std::string> debPaths = listDebPaths(root);
        Logger::info("  [Store] synced " + std::to_string(debPaths.size()) + " package(s) to device");
    } else {
        Logger::info("  [Store] updated apt sources on device (no mirror upload)");
    }
    result.success = true;
    return result;
}

bool installPackageOnDevice(const RamdiskConnectOptions& connect, const std::string& jbroot,
                            const std::string& packageName, bool allowMutation, std::string* error) {
    if (packageName.empty()) {
        if (error != nullptr) {
            *error = "empty package name";
        }
        return false;
    }
    if (!allowMutation) {
        Logger::info("  [Store] dry-run install " + packageName);
        return true;
    }

    RamdiskClient client(connect);
    std::string probeMsg;
    if (!client.probe(&probeMsg)) {
        if (error != nullptr) {
            *error = probeMsg.empty() ? "SSH probe failed" : probeMsg;
        }
        return false;
    }

    const std::string root = jbroot.empty() ? primitives::RootlessLayout::resolveJbroot() : jbroot;
    const std::string aptBin = root + "/usr/bin/apt";
    const std::string dpkgBin = root + "/usr/bin/dpkg";
    const std::string poolRoot = deviceStorePath(root) + "/pool";

    std::ostringstream cmd;
    cmd << "PKG=" << shellQuote(packageName) << "; ";
    cmd << "if test -x " << shellQuote(aptBin) << "; then ";
    cmd << shellQuote(aptBin) << " install -y --allow-unauthenticated \"$PKG\"; ";
    cmd << "elif test -x " << shellQuote(dpkgBin) << "; then ";
    cmd << "DEB=$(find " << shellQuote(poolRoot) << " -name \"*$PKG*.deb\" 2>/dev/null | head -1); ";
    cmd << "test -n \"$DEB\" && " << shellQuote(dpkgBin) << " -i \"$DEB\"; ";
    cmd << "else exit 127; fi";

    const RamdiskCommandResult run = client.exec(cmd.str());
    if (run.exitCode != 0) {
        if (error != nullptr) {
            *error = run.stderrText.empty() ? "install command failed" : run.stderrText;
        }
        return false;
    }
    if (!run.stdoutText.empty()) {
        Logger::info(run.stdoutText);
    }
    Logger::info("  [Store] installed " + packageName);
    return true;
}

bool listInstalledPackagesOnDevice(const RamdiskConnectOptions& connect,
                                   std::vector<std::string>* packages, std::string* error) {
    if (packages == nullptr) {
        if (error != nullptr) {
            *error = "null packages vector";
        }
        return false;
    }
    packages->clear();

    RamdiskClient client(connect);
    std::string probeMsg;
    if (!client.probe(&probeMsg)) {
        if (error != nullptr) {
            *error = probeMsg.empty() ? "SSH probe failed" : probeMsg;
        }
        return false;
    }

    const std::string jbroot = primitives::RootlessLayout::resolveJbroot();
    const std::string dpkgBin = jbroot + "/usr/bin/dpkg";
    const std::string cmd = "test -x " + shellQuote(dpkgBin) + " && " + shellQuote(dpkgBin) + " -l";
    const RamdiskCommandResult run = client.exec(cmd);
    if (run.exitCode != 0) {
        if (error != nullptr) {
            *error = run.stderrText.empty() ? "dpkg -l failed" : run.stderrText;
        }
        return false;
    }

    std::istringstream lines(run.stdoutText);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.size() < 4 || line.compare(0, 2, "ii") != 0) {
            continue;
        }
        size_t start = line.find_first_not_of(' ', 2);
        if (start == std::string::npos) {
            continue;
        }
        size_t end = line.find(' ', start);
        if (end == std::string::npos) {
            packages->push_back(line.substr(start));
        } else {
            packages->push_back(line.substr(start, end - start));
        }
    }
    return true;
}

} /* namespace store */
} /* namespace PP */
