/*
 * DpkgStoreSync.cpp
 */

#include "store/DpkgStoreSync.h"
#include "primitives/RootlessLayout.h"
#include "RamdiskClient.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <unistd.h>
#include <vector>

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

} /* anonymous */

StoreSyncResult syncStoreToDevice(const RamdiskConnectOptions& connect, const std::string& storeRoot,
                                  bool allowMutation) {
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

    if (!allowMutation) {
        Logger::info("  [Store] dry-run sync → " + result.deviceStorePath);
        result.success = true;
        return result;
    }

    RamdiskClient client(connect);
    std::string probeMsg;
    if (!client.probe(&probeMsg)) {
        result.error = probeMsg.empty() ? "SSH probe failed" : probeMsg;
        return result;
    }

    if (!ensureRemoteDir(client, result.deviceStorePath)) {
        result.error = "failed to create device store dir";
        return result;
    }

    const std::string aptDir = primitives::RootlessLayout::jbPath("/etc/apt/sources.list.d", jbroot);
    if (!ensureRemoteDir(client, aptDir)) {
        result.error = "failed to create apt sources dir";
        return result;
    }

    const std::vector<std::string> indexFiles = {
        built.packagesPath,
        built.packagesGzPath,
        built.packagesBz2Path,
        built.releasePath,
        joinPath(root, "Packages"),
    };
    for (size_t i = 0; i < indexFiles.size(); ++i) {
        if (indexFiles[i].empty()) {
            continue;
        }
        if (!uploadStoreFile(client, indexFiles[i], root, result.deviceStorePath)) {
            result.error = "failed to upload " + relativeFromStoreRoot(root, indexFiles[i]);
            return result;
        }
    }

    const std::vector<std::string> debPaths = listDebPaths(root);
    for (size_t i = 0; i < debPaths.size(); ++i) {
        if (!uploadStoreFile(client, debPaths[i], root, result.deviceStorePath)) {
            const size_t slash = debPaths[i].find_last_of('/');
            const std::string base = slash == std::string::npos ? debPaths[i] : debPaths[i].substr(slash + 1);
            result.error = "failed to upload " + base;
            return result;
        }
    }

    const std::string sourceLine = aptSourceLine(jbroot) + "\n";
    const std::string tmpSources = "/tmp/pp-purplepois0n-sources-" + std::to_string(getpid()) + ".list";
    {
        std::ofstream out(tmpSources.c_str(), std::ios::trunc);
        if (!out) {
            result.error = "failed to write temp sources list";
            return result;
        }
        out << sourceLine;
    }
    if (!client.uploadFile(tmpSources, result.sourcesListPath)) {
        result.error = "failed to install apt source list";
        return result;
    }
    unlink(tmpSources.c_str());

    const std::string aptBin = jbroot + "/usr/bin/apt";
    const std::string updateCmd = "test -x " + shellQuote(aptBin) + " && " + shellQuote(aptBin) +
                                " update 2>/dev/null || true";
    client.exec(updateCmd);

    Logger::info("  [Store] synced " + std::to_string(debPaths.size()) + " package(s) to device");
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

} /* namespace store */
} /* namespace PP */
