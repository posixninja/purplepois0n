/*
 * BackupProtocol.h
 *
 * Offline backup format taxonomy: index protocol (v1 mbdb/plist vs v2 Manifest.db)
 * and storage layout (v1 flat hash paths vs v2 sharded ab/hash paths).
 */

#ifndef BACKUP_PROTOCOL_H_
#define BACKUP_PROTOCOL_H_

#include <string>

namespace PP {

/** Manifest index era: mobilebackup (v1) vs mobilebackup2 SQLite (v2). */
enum class BackupIndexProtocol {
    Unknown,
    V1,  ///< Manifest.mbdb or Manifest.plist Files dict
    V2   ///< Manifest.db (SQLite Files table)
};

/** On-disk hash path layout from Status.plist Version (2.x flat, 3.x+ sharded). */
enum class BackupStorageProtocol {
    Unknown,
    V1,  ///< Files at backupRoot/<hash>
    V2   ///< Files at backupRoot/<hash[0:2]>/<hash>
};

struct BackupProtocolInfo {
    BackupIndexProtocol index = BackupIndexProtocol::Unknown;
    BackupStorageProtocol storage = BackupStorageProtocol::Unknown;
    std::string statusVersion;
};

BackupStorageProtocol storageFromStatusVersion(const std::string& version);
BackupIndexProtocol indexFromManifestTypeName(const std::string& manifestTypeName);
std::string backupIndexProtocolName(BackupIndexProtocol protocol);
std::string backupStorageProtocolName(BackupStorageProtocol protocol);
std::string resolveBackupHashPath(const std::string& backupRoot,
                                  const std::string& hash,
                                  BackupStorageProtocol storage);

} /* namespace PP */

#endif /* BACKUP_PROTOCOL_H_ */
