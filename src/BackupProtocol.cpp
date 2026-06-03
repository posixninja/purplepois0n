/*
 * BackupProtocol.cpp
 */

#include "BackupProtocol.h"
#include <cstdlib>
#include <sys/stat.h>

namespace PP {

namespace {

bool pathExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

} /* anonymous namespace */

BackupStorageProtocol storageFromStatusVersion(const std::string& version) {
    if (version.empty()) {
        return BackupStorageProtocol::Unknown;
    }

    char* end = nullptr;
    const double value = std::strtod(version.c_str(), &end);
    if (end == version.c_str()) {
        return BackupStorageProtocol::Unknown;
    }
    if (value >= 3.0) {
        return BackupStorageProtocol::V2;
    }
    if (value >= 2.0) {
        return BackupStorageProtocol::V1;
    }
    return BackupStorageProtocol::Unknown;
}

BackupIndexProtocol indexFromManifestTypeName(const std::string& manifestTypeName) {
    if (manifestTypeName == "Manifest.db") {
        return BackupIndexProtocol::V2;
    }
    if (manifestTypeName == "Manifest.mbdb" || manifestTypeName == "Manifest.plist") {
        return BackupIndexProtocol::V1;
    }
    return BackupIndexProtocol::Unknown;
}

std::string backupIndexProtocolName(BackupIndexProtocol protocol) {
    switch (protocol) {
        case BackupIndexProtocol::V1:
            return "v1 (mbdb/plist index)";
        case BackupIndexProtocol::V2:
            return "v2 (Manifest.db index)";
        default:
            return "unknown";
    }
}

std::string backupStorageProtocolName(BackupStorageProtocol protocol) {
    switch (protocol) {
        case BackupStorageProtocol::V1:
            return "v1 (flat hash paths)";
        case BackupStorageProtocol::V2:
            return "v2 (sharded hash paths)";
        default:
            return "unknown";
    }
}

std::string resolveBackupHashPath(const std::string& backupRoot,
                                  const std::string& hash,
                                  BackupStorageProtocol storage) {
    if (hash.length() < 2) {
        return "";
    }

    const std::string flat = backupRoot + "/" + hash;
    const std::string sharded = backupRoot + "/" + hash.substr(0, 2) + "/" + hash;

    if (storage == BackupStorageProtocol::V1) {
        if (pathExists(flat)) {
            return flat;
        }
        if (pathExists(sharded)) {
            return sharded;
        }
        return flat;
    }

    if (storage == BackupStorageProtocol::V2) {
        if (pathExists(sharded)) {
            return sharded;
        }
        if (pathExists(flat)) {
            return flat;
        }
        return sharded;
    }

    if (pathExists(sharded)) {
        return sharded;
    }
    if (pathExists(flat)) {
        return flat;
    }
    return sharded;
}

} /* namespace PP */
