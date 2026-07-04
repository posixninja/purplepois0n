/*
 * DpkgStoreSync.h
 *
 * Push host dpkg store to jailbroken device via RamdiskClient SSH.
 */

#ifndef STORE_DPKG_STORE_SYNC_H_
#define STORE_DPKG_STORE_SYNC_H_

#include "RamdiskTypes.h"
#include "store/DpkgStore.h"

#include <string>
#include <vector>

namespace PP {
namespace store {

/** How store sync interacts with device apt sources and local mirror upload. */
enum class StoreSyncMode {
    /** Upload pool + dists; write file:// sources (dev SSH default). */
    File,
    /** Skip upload and sources.list — device uses HTTPS repo (prod). */
    Https,
    /** Write sources.list only (file:// or PURPLEPOIS0N_REPO_URL); skip upload. */
    SourcesOnly,
};

StoreSyncMode parseStoreSyncMode(const std::string& mode);
const char* storeSyncModeLabel(StoreSyncMode mode);

StoreSyncResult syncStoreToDevice(const RamdiskConnectOptions& connect,
                                  const std::string& storeRoot,
                                  bool allowMutation,
                                  StoreSyncMode mode = StoreSyncMode::File);

bool installPackageOnDevice(const RamdiskConnectOptions& connect,
                            const std::string& jbroot,
                            const std::string& packageName,
                            bool allowMutation,
                            std::string* error);

/** List installed package names via SSH dpkg -l (ii state). */
bool listInstalledPackagesOnDevice(const RamdiskConnectOptions& connect,
                                   std::vector<std::string>* packages,
                                   std::string* error);

} /* namespace store */
} /* namespace PP */

#endif /* STORE_DPKG_STORE_SYNC_H_ */
