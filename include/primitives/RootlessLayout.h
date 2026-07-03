/*
 * RootlessLayout.h
 *
 * libroot-style jbroot path helpers for Generation 6 rootless jailbreaks.
 * System volume stays sealed; jailbreak state lives under /var/jb (or roothide prefix).
 */

#ifndef PRIMITIVES_ROOTLESS_LAYOUT_H_
#define PRIMITIVES_ROOTLESS_LAYOUT_H_

#include <string>
#include <vector>

namespace PP {
namespace primitives {

/** Known marker paths relative to jbroot (presence ⇒ bootstrap flavor). */
struct RootlessMarker {
    const char* relativePath;
    const char* label;
    bool required;
};

/** Result of on-device or host-side layout validation. */
struct RootlessProbeResult {
    bool sshReachable = false;
    std::string jbroot;
    bool jbrootExists = false;
    bool layoutComplete = false;
    bool bindMountsVisible = false;
    bool rootfulWritableSystem = false;
    std::string bootstrapFlavor;
    std::vector<std::string> foundMarkers;
    std::vector<std::string> missingRequired;
};

class RootlessLayout {
public:
    static constexpr const char* kDefaultJbroot = "/var/jb";

    /** PURPLEPOIS0N_JBROOT or default /var/jb. */
    static std::string resolveJbroot();

    /** Map a rootfs path (e.g. /usr/bin/bash) into jbroot space. */
    static std::string jbPath(const std::string& rootfsPath,
                              const std::string& jbroot = std::string());

    /** Strip jbroot prefix when present; otherwise return path unchanged. */
    static std::string rootfsPath(const std::string& path, const std::string& jbroot = std::string());

    /** True when path is already under jbroot. */
    static bool isUnderJbroot(const std::string& path, const std::string& jbroot = std::string());

    /** Standard Dopamine / palera1n / Procursus markers under jbroot. */
    static const std::vector<RootlessMarker>& standardMarkers();

    /** Host-side: validate marker list against a local directory tree (offline fixtures). */
    static RootlessProbeResult probeLocalTree(const std::string& jbrootDir);

    static bool pathExists(const std::string& path);
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_ROOTLESS_LAYOUT_H_ */
