/*
 * DpkgStore.h
 *
 * Host-side dpkg/apt repository for rootless /var/jb bootstraps.
 * Generates Packages/Release indices and syncs to a jailbroken device over SSH.
 */

#ifndef STORE_DPKG_STORE_H_
#define STORE_DPKG_STORE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace PP {
namespace store {

struct DebControl {
    std::string package;
    std::string version;
    std::string architecture;
    std::string maintainer;
    std::string description;
    std::string filename;
    uint64_t size = 0;
    std::string sha256;
};

struct StoreBuildResult {
    bool success = false;
    std::string storeRoot;
    std::string packagesPath;
    std::string packagesGzPath;
    std::string packagesBz2Path;
    std::string releasePath;
    size_t packageCount = 0;
    std::string error;
};

struct StorePublishResult {
    bool success = false;
    std::string publishRoot;
    std::string error;
};

struct StoreSyncResult {
    bool success = false;
    std::string deviceStorePath;
    std::string sourcesListPath;
    std::string error;
};

/** PURPLEPOIS0N_STORE_ROOT or <cwd>/store */
std::string resolveStoreRoot();

/** jbroot-relative path where the repo is published on device. */
std::string deviceStoreRelativePath();

/** Full device path: /var/jb/var/mobile/purplepois0n-store by default. */
std::string deviceStorePath(const std::string& jbroot = std::string());

std::string deviceSourcesListPath(const std::string& jbroot = std::string());

/** Create pool/ + dists/ layout under store root. */
bool initRepository(const std::string& storeRoot, std::string* error);

/** Parse DEBIAN/control from a .deb (uses ar+tar on host when dpkg-deb missing). */
bool readDebControl(const std::string& debPath, DebControl* control, std::string* error);

/** Scan pool/ and write dists/.../Packages + Release (+ .gz/.bz2). */
StoreBuildResult buildRepository(const std::string& storeRoot);

/** Copy built repo tree to @p publishRoot for static HTTPS hosting. */
StorePublishResult publishRepository(const std::string& storeRoot, const std::string& publishRoot);

/** Apt suite name (dists subdirectory). */
const char* repositorySuite();

/** One line for /var/jb/etc/apt/sources.list.d/purplepois0n.list (file:// device mirror). */
std::string aptSourceLine(const std::string& jbroot = std::string());

/** Canonical HTTPS repo URL from PURPLEPOIS0N_REPO_URL (trailing slash normalized). */
std::string resolveRepoUrl();

/** Apt source line for HTTPS prod repo (uses resolveRepoUrl()). */
std::string aptHttpsSourceLine();

/** Markdown/nginx hosting hint aligned with deploy-https-repo.sh. */
std::string hostingReadmeText(const std::string& publishRoot = std::string());

std::vector<std::string> listDebPaths(const std::string& storeRoot);

/** Copy a .deb into pool/main/<first>/<Package>/ and return false on error. */
bool addDebToPool(const std::string& storeRoot, const std::string& debPath, std::string* error);

} /* namespace store */
} /* namespace PP */

#endif /* STORE_DPKG_STORE_H_ */
