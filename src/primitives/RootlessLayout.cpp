/*
 * RootlessLayout.cpp
 */

#include "primitives/RootlessLayout.h"
#include "EnvUtil.h"

#include <sys/stat.h>
#include <unistd.h>

namespace PP {
namespace primitives {

namespace {

bool isDir(const std::string& path) {
    struct stat st = {};
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool isPresent(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

std::string trimTrailingSlash(std::string path) {
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

} /* anonymous */

std::string RootlessLayout::resolveJbroot() {
    const std::string env = PP::envOrEmpty("PURPLEPOIS0N_JBROOT");
    if (!env.empty()) {
        return trimTrailingSlash(env);
    }
    return kDefaultJbroot;
}

std::string RootlessLayout::jbPath(const std::string& rootfsPath, const std::string& jbroot) {
    const std::string root = jbroot.empty() ? resolveJbroot() : trimTrailingSlash(jbroot);
    if (rootfsPath.empty() || rootfsPath == "/") {
        return root;
    }
    if (isUnderJbroot(rootfsPath, root)) {
        return rootfsPath;
    }
    if (rootfsPath[0] == '/') {
        return root + rootfsPath;
    }
    return root + "/" + rootfsPath;
}

std::string RootlessLayout::rootfsPath(const std::string& path, const std::string& jbroot) {
    const std::string root = jbroot.empty() ? resolveJbroot() : trimTrailingSlash(jbroot);
    if (path.size() >= root.size() && path.compare(0, root.size(), root) == 0) {
        const std::string rest = path.substr(root.size());
        return rest.empty() ? std::string("/") : rest;
    }
    return path;
}

bool RootlessLayout::isUnderJbroot(const std::string& path, const std::string& jbroot) {
    const std::string root = jbroot.empty() ? resolveJbroot() : trimTrailingSlash(jbroot);
    if (path == root) {
        return true;
    }
    if (path.size() <= root.size()) {
        return false;
    }
    return path.compare(0, root.size(), root) == 0 && path[root.size()] == '/';
}

const std::vector<RootlessMarker>& RootlessLayout::standardMarkers() {
    static const RootlessMarker markers[] = {
        {".installed_dopamine", "dopamine", false},
        {".palera1n-rootless", "palera1n", false},
        {".procursus_strapped", "procursus", false},
        {"basebin/jbctl", "jbctl", false},
        {"usr/bin/dpkg", "dpkg", true},
        {"usr/bin/bash", "bash", false},
        {"Library/LaunchDaemons", "launchdaemons", false},
        {"Library/Frameworks", "frameworks", false},
    };
    static const std::vector<RootlessMarker> list(
        markers, markers + sizeof(markers) / sizeof(markers[0]));
    return list;
}

bool RootlessLayout::pathExists(const std::string& path) {
    return isPresent(path);
}

RootlessProbeResult RootlessLayout::probeLocalTree(const std::string& jbrootDir) {
    RootlessProbeResult result;
    result.jbroot = trimTrailingSlash(jbrootDir);
    result.jbrootExists = isDir(result.jbroot);
    if (!result.jbrootExists) {
        return result;
    }

    for (const RootlessMarker& marker : standardMarkers()) {
        const std::string full = result.jbroot + "/" + marker.relativePath;
        if (isPresent(full)) {
            result.foundMarkers.push_back(marker.label);
            if (marker.label == std::string("dopamine")) {
                result.bootstrapFlavor = "dopamine";
            } else if (marker.label == std::string("palera1n")) {
                result.bootstrapFlavor = "palera1n";
            } else if (marker.label == std::string("procursus") &&
                       result.bootstrapFlavor.empty()) {
                result.bootstrapFlavor = "procursus";
            }
        } else if (marker.required) {
            result.missingRequired.push_back(marker.relativePath);
        }
    }

    result.layoutComplete = result.missingRequired.empty() && !result.foundMarkers.empty();
    return result;
}

} /* namespace primitives */
} /* namespace PP */
