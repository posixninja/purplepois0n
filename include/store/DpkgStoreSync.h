/*
 * DpkgStoreSync.h
 *
 * Push host dpkg store to jailbroken device via RamdiskClient SSH.
 */

#ifndef STORE_DPKG_STORE_SYNC_H_
#define STORE_DPKG_STORE_SYNC_H_

#include "RamdiskTypes.h"
#include "store/DpkgStore.h"

namespace PP {
namespace store {

StoreSyncResult syncStoreToDevice(const RamdiskConnectOptions& connect,
                                  const std::string& storeRoot,
                                  bool allowMutation);

bool installPackageOnDevice(const RamdiskConnectOptions& connect,
                            const std::string& jbroot,
                            const std::string& packageName,
                            bool allowMutation,
                            std::string* error);

} /* namespace store */
} /* namespace PP */

#endif /* STORE_DPKG_STORE_SYNC_H_ */
