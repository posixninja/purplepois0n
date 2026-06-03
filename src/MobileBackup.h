/*
 * MobileBackup.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef MOBILE_BACKUP_H_
#define MOBILE_BACKUP_H_

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include <plist/plist.h>
#include "MbdbParser.h"
#include "ManifestDbParser.h"
#include "BackupProtocol.h"

namespace PP {

/**
 * @enum ManifestType
 * @brief On-disk manifest format for an iTunes/Finder backup tree
 */
enum class ManifestType {
    Unknown,
    Plist,      ///< Manifest.plist (iOS 4+ plist era)
    Mbdb,       ///< Manifest.mbdb (iOS 5–9 absinthe era)
    SqliteDb    ///< Manifest.db (iOS 10+)
};

/**
 * @struct BackupFileInfo
 * @brief Information about a file in the backup
 */
struct BackupFileInfo {
    std::string domain;         ///< Domain (e.g., "HomeDomain", "SystemPreferencesDomain")
    std::string path;            ///< File path
    std::string hash;            ///< File hash (SHA1)
    uint64_t size;               ///< File size in bytes
    std::string encryptionKey;   ///< Encryption key (if encrypted)
    bool isEncrypted;            ///< Whether the file is encrypted
    bool hasMetadata;            ///< Size/times resolved from manifest blob
    uint64_t mtime;              ///< Modification time
    uint64_t ctime;              ///< Creation time
};

/**
 * @class MobileBackup
 * @brief Parser for iOS mobile backup files
 * 
 * iOS backups are stored in a specific format with a manifest file
 * (Info.plist or Manifest.plist) and individual files hashed by their
 * SHA1 hash. This class can parse backup information and extract files.
 */
class MobileBackup {
public:
    /**
     * @brief Construct a MobileBackup instance
     * @param backupPath Path to the backup directory
     * @throws std::runtime_error if the backup cannot be opened
     */
    explicit MobileBackup(const std::string& backupPath);
    
    ~MobileBackup();

    /**
     * @brief Check if the backup is valid
     * @return True if valid, false otherwise
     */
    bool isValid() const;

    /**
     * @brief Get the device UDID from the backup
     * @return UDID string
     */
    std::string getUDID() const;

    /**
     * @brief Get the device name
     * @return Device name
     */
    std::string getDeviceName() const;

    /**
     * @brief Get the iOS version
     * @return iOS version string
     */
    std::string getIOSVersion() const;

    /**
     * @brief Get the backup date
     * @return Backup date as a string
     */
    std::string getBackupDate() const;

    /**
     * @brief Get all files in the backup
     * @return Vector of BackupFileInfo structures
     */
    std::vector<BackupFileInfo> getAllFiles() const;

    /**
     * @brief Get files by domain
     * @param domain Domain to filter by
     * @return Vector of BackupFileInfo structures
     */
    std::vector<BackupFileInfo> getFilesByDomain(const std::string& domain) const;

    /**
     * @brief Find a file by path
     * @param domain Domain of the file
     * @param path Path to the file
     * @return File info if found, nullptr otherwise
     */
    std::unique_ptr<BackupFileInfo> findFile(const std::string& domain, const std::string& path) const;

    /**
     * @brief Extract a file from the backup
     * @param fileInfo File information
     * @param outputPath Path to save the extracted file
     * @return True if successful, false otherwise
     */
    bool extractFile(const BackupFileInfo& fileInfo, const std::string& outputPath) const;

    /**
     * @brief Extract a file by domain and path
     * @param domain Domain of the file
     * @param path Path to the file
     * @param outputPath Path to save the extracted file
     * @return True if successful, false otherwise
     */
    bool extractFile(const std::string& domain, const std::string& path, const std::string& outputPath) const;

    /**
     * @brief Get all available domains
     * @return Vector of domain names
     */
    std::vector<std::string> getDomains() const;

    /**
     * @brief Check if the backup is encrypted
     * @return True if encrypted, false otherwise
     */
    bool isEncrypted() const;

    /** Manifest on-disk format detected for this backup. */
    ManifestType getManifestType() const;

    /** Human-readable manifest type for CLI output. */
    std::string getManifestTypeName() const;

    /** Aggregate stats for CLI / research summaries. */
    struct Stats {
        size_t totalEntries = 0;
        size_t entriesWithMetadata = 0;
        size_t encryptedEntries = 0;
        size_t directoryEntries = 0;
    };
    Stats getStats() const;

    /** Index + storage protocol detected for this backup tree. */
    BackupProtocolInfo getProtocolInfo() const;

private:
    void detectManifest();
    void parseManifest();
    void parseManifestPlist();
    void parseManifestMbdb();
    void parseManifestSqlite();
    void parseManifestPlistEncryption();
    void parseStatusPlist();
    void parseInfoPlist();
    void finalizeProtocolDetection();
    void indexFileInfo(const BackupFileInfo& info);
    BackupFileInfo fileInfoFromMbdbRecord(const MbdbRecord& record) const;
    BackupFileInfo fileInfoFromManifestDbRecord(const ManifestDbRecord& record) const;
    BackupFileInfo parseFileInfo(plist_t fileDict, const std::string& domain);
    std::string getPlistString(plist_t node, const char* key) const;
    uint64_t getPlistUInt64(plist_t node, const char* key) const;
    bool getPlistBool(plist_t node, const char* key) const;
    std::string getHashPath(const std::string& hash) const;
    bool decryptFile(const std::string& hash, const std::string& key, const std::string& outputPath) const;

    std::string m_backupPath;
    std::string m_manifestPath;
    ManifestType m_manifestType;
    bool m_valid;
    bool m_encrypted;
    std::string m_udid;
    std::string m_deviceName;
    std::string m_iosVersion;
    std::string m_backupDate;
    BackupIndexProtocol m_indexProtocol;
    BackupStorageProtocol m_storageProtocol;
    std::string m_statusVersion;
    std::vector<BackupFileInfo> m_files;
    std::map<std::string, std::vector<BackupFileInfo>> m_filesByDomain;
    plist_t m_manifest;
    plist_t m_infoPlist;
};

} /* namespace PP */

#endif /* MOBILE_BACKUP_H_ */

