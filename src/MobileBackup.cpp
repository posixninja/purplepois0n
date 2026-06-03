/*
 * MobileBackup.cpp
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#include "MobileBackup.h"
#include "MbdbParser.h"
#include "ManifestDbParser.h"
#include "BackupProtocol.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <dirent.h>
#include "Logger.h"

namespace PP {

namespace {

std::string manifestTypeName(ManifestType type) {
    switch (type) {
        case ManifestType::Plist:
            return "Manifest.plist";
        case ManifestType::Mbdb:
            return "Manifest.mbdb";
        case ManifestType::SqliteDb:
            return "Manifest.db";
        default:
            return "unknown";
    }
}

} /* anonymous namespace */

MobileBackup::MobileBackup(const std::string& backupPath)
    : m_backupPath(backupPath), m_manifestType(ManifestType::Unknown), m_valid(false),
      m_encrypted(false), m_indexProtocol(BackupIndexProtocol::Unknown),
      m_storageProtocol(BackupStorageProtocol::Unknown), m_manifest(nullptr), m_infoPlist(nullptr) {
    detectManifest();
    parseInfoPlist();
    parseStatusPlist();
    if (m_manifestType == ManifestType::SqliteDb) {
        parseManifestPlistEncryption();
    }
    parseManifest();
    finalizeProtocolDetection();
}

MobileBackup::~MobileBackup() {
    if (m_manifest) {
        plist_free(m_manifest);
    }
    if (m_infoPlist) {
        plist_free(m_infoPlist);
    }
}

bool MobileBackup::isValid() const {
    return m_valid;
}

std::string MobileBackup::getUDID() const {
    return m_udid;
}

std::string MobileBackup::getDeviceName() const {
    return m_deviceName;
}

std::string MobileBackup::getIOSVersion() const {
    return m_iosVersion;
}

std::string MobileBackup::getBackupDate() const {
    return m_backupDate;
}

ManifestType MobileBackup::getManifestType() const {
    return m_manifestType;
}

std::string MobileBackup::getManifestTypeName() const {
    return manifestTypeName(m_manifestType);
}

MobileBackup::Stats MobileBackup::getStats() const {
    Stats stats;
    stats.totalEntries = m_files.size();
    for (const auto& file : m_files) {
        if (file.hasMetadata) {
            ++stats.entriesWithMetadata;
        }
        if (file.isEncrypted) {
            ++stats.encryptedEntries;
        }
        if (file.hash.empty() || file.path.empty()) {
            ++stats.directoryEntries;
        }
    }
    return stats;
}

BackupProtocolInfo MobileBackup::getProtocolInfo() const {
    BackupProtocolInfo info;
    info.index = m_indexProtocol;
    info.storage = m_storageProtocol;
    info.statusVersion = m_statusVersion;
    return info;
}

std::vector<BackupFileInfo> MobileBackup::getAllFiles() const {
    return m_files;
}

std::vector<BackupFileInfo> MobileBackup::getFilesByDomain(const std::string& domain) const {
    auto it = m_filesByDomain.find(domain);
    if (it != m_filesByDomain.end()) {
        return it->second;
    }
    return std::vector<BackupFileInfo>();
}

std::unique_ptr<BackupFileInfo> MobileBackup::findFile(const std::string& domain, const std::string& path) const {
    for (const auto& file : m_files) {
        if (file.domain == domain && file.path == path) {
            auto info = std::make_unique<BackupFileInfo>();
            *info = file;
            return info;
        }
    }
    return nullptr;
}

bool MobileBackup::extractFile(const BackupFileInfo& fileInfo, const std::string& outputPath) const {
    if (fileInfo.hash.empty()) {
        Logger::error("Cannot extract entry without file hash (directory or symlink?)");
        return false;
    }

    std::string hashPath = getHashPath(fileInfo.hash);
    
    std::ifstream input(hashPath, std::ios::binary);
    if (!input.is_open()) {
        Logger::error("Failed to open backup file: " + hashPath);
        return false;
    }

    std::ofstream output(outputPath, std::ios::binary);
    if (!output.is_open()) {
        Logger::error("Failed to create output file: " + outputPath);
        return false;
    }

    if (fileInfo.isEncrypted) {
        return decryptFile(fileInfo.hash, fileInfo.encryptionKey, outputPath);
    }

    output << input.rdbuf();
    return true;
}

bool MobileBackup::extractFile(const std::string& domain, const std::string& path, const std::string& outputPath) const {
    auto fileInfo = findFile(domain, path);
    if (!fileInfo) {
        Logger::error("File not found: " + domain + "/" + path);
        return false;
    }

    return extractFile(*fileInfo, outputPath);
}

std::vector<std::string> MobileBackup::getDomains() const {
    std::vector<std::string> domains;
    for (const auto& pair : m_filesByDomain) {
        domains.push_back(pair.first);
    }
    return domains;
}

bool MobileBackup::isEncrypted() const {
    return m_encrypted;
}

void MobileBackup::detectManifest() {
    struct stat st;
    const std::string dbPath = m_backupPath + "/Manifest.db";
    const std::string mbdbPath = m_backupPath + "/Manifest.mbdb";
    const std::string plistPath = m_backupPath + "/Manifest.plist";

    if (stat(dbPath.c_str(), &st) == 0) {
        m_manifestPath = dbPath;
        m_manifestType = ManifestType::SqliteDb;
        return;
    }
    if (stat(mbdbPath.c_str(), &st) == 0) {
        m_manifestPath = mbdbPath;
        m_manifestType = ManifestType::Mbdb;
        return;
    }
    if (stat(plistPath.c_str(), &st) == 0) {
        m_manifestPath = plistPath;
        m_manifestType = ManifestType::Plist;
        return;
    }

    m_manifestType = ManifestType::Unknown;
}

void MobileBackup::parseStatusPlist() {
    const std::string statusPath = m_backupPath + "/Status.plist";
    struct stat st;
    if (stat(statusPath.c_str(), &st) != 0) {
        return;
    }

    plist_t status = nullptr;
    plist_format_t format = PLIST_FORMAT_NONE;
    if (plist_read_from_file(statusPath.c_str(), &status, &format) != PLIST_ERR_SUCCESS ||
        status == nullptr) {
        return;
    }

    m_statusVersion = getPlistString(status, "Version");
    m_storageProtocol = storageFromStatusVersion(m_statusVersion);
    plist_free(status);
}

void MobileBackup::finalizeProtocolDetection() {
    m_indexProtocol = indexFromManifestTypeName(getManifestTypeName());
    if (m_storageProtocol == BackupStorageProtocol::Unknown) {
        if (m_indexProtocol == BackupIndexProtocol::V1) {
            m_storageProtocol = BackupStorageProtocol::V1;
        } else if (m_indexProtocol == BackupIndexProtocol::V2) {
            m_storageProtocol = BackupStorageProtocol::V2;
        }
    }
}

void MobileBackup::parseManifest() {
    switch (m_manifestType) {
        case ManifestType::Plist:
            parseManifestPlist();
            break;
        case ManifestType::Mbdb:
            parseManifestMbdb();
            break;
        case ManifestType::SqliteDb:
            parseManifestSqlite();
            break;
        default:
            Logger::warn("No Manifest.db, Manifest.mbdb, or Manifest.plist found in backup");
            break;
    }
}

void MobileBackup::parseManifestPlist() {
    plist_format_t format = PLIST_FORMAT_NONE;
    if (plist_read_from_file(m_manifestPath.c_str(), &m_manifest, &format) != PLIST_ERR_SUCCESS ||
        m_manifest == nullptr) {
        Logger::warn("Failed to read Manifest.plist");
        return;
    }

    plist_t files = plist_dict_get_item(m_manifest, "Files");
    if (!files || plist_get_node_type(files) != PLIST_DICT) {
        Logger::warn("No Files dictionary in manifest");
        return;
    }

    plist_dict_iter iter = nullptr;
    plist_dict_new_iter(files, &iter);
    
    char* key = nullptr;
    plist_t fileDict = nullptr;
    plist_dict_next_item(files, iter, &key, &fileDict);
    
    while (fileDict) {
        plist_t domainNode = plist_dict_get_item(fileDict, "Domain");
        std::string domain = getPlistString(domainNode, nullptr);
        
        if (domain.empty()) {
            domain = "Unknown";
        }

        indexFileInfo(parseFileInfo(fileDict, domain));
        
        free(key);
        plist_dict_next_item(files, iter, &key, &fileDict);
    }
    
    free(iter);
    m_valid = true;
}

void MobileBackup::parseManifestMbdb() {
    MbdbParser parser;
    if (!parser.parseFile(m_manifestPath)) {
        Logger::warn("Failed to parse Manifest.mbdb");
        return;
    }

    const std::vector<MbdbRecord>& records = parser.records();
    for (size_t i = 0; i < records.size(); ++i) {
        indexFileInfo(fileInfoFromMbdbRecord(records[i]));
    }

    m_valid = !m_files.empty();
}

void MobileBackup::parseManifestPlistEncryption() {
    const std::string plistPath = m_backupPath + "/Manifest.plist";
    struct stat st;
    if (stat(plistPath.c_str(), &st) != 0) {
        return;
    }

    plist_t manifest = nullptr;
    plist_format_t format = PLIST_FORMAT_NONE;
    if (plist_read_from_file(plistPath.c_str(), &manifest, &format) != PLIST_ERR_SUCCESS ||
        manifest == nullptr) {
        return;
    }

    if (getPlistBool(manifest, "IsEncrypted")) {
        m_encrypted = true;
    }

    plist_free(manifest);
}

void MobileBackup::parseManifestSqlite() {
    ManifestDbParser parser;
    if (!parser.parseFile(m_manifestPath)) {
        Logger::warn("Failed to parse Manifest.db");
        return;
    }

    const std::vector<ManifestDbRecord>& records = parser.records();
    for (size_t i = 0; i < records.size(); ++i) {
        indexFileInfo(fileInfoFromManifestDbRecord(records[i]));
    }

    m_valid = !m_files.empty();
}

BackupFileInfo MobileBackup::fileInfoFromManifestDbRecord(const ManifestDbRecord& record) const {
    BackupFileInfo info;
    info.domain = record.domain.empty() ? "Unknown" : record.domain;
    info.path = record.relativePath;
    info.hash = record.fileID;
    info.size = record.size;
    info.mtime = record.mtime;
    info.ctime = record.ctime;
    info.isEncrypted = m_encrypted && record.isEncryptedEntry;
    info.encryptionKey = record.encryptionKey;
    info.hasMetadata = record.hasFileMetadata;
    return info;
}

BackupFileInfo MobileBackup::fileInfoFromMbdbRecord(const MbdbRecord& record) const {
    BackupFileInfo info;
    info.domain = record.domain.empty() ? "Unknown" : record.domain;
    info.path = record.path;
    info.hash = MbdbParser::hashToHex(record.dataHash);
    info.size = record.size;
    info.mtime = record.mtime;
    info.ctime = record.ctime;
    info.isEncrypted = m_encrypted && record.flag != 0;
    info.hasMetadata = !record.path.empty();
    return info;
}

void MobileBackup::indexFileInfo(const BackupFileInfo& info) {
    m_files.push_back(info);
    m_filesByDomain[info.domain].push_back(info);
}

void MobileBackup::parseInfoPlist() {
    std::string infoPath = m_backupPath + "/Info.plist";
    struct stat st;
    if (stat(infoPath.c_str(), &st) != 0) {
        Logger::warn("Info.plist not found");
        return;
    }

    plist_format_t format = PLIST_FORMAT_NONE;
    if (plist_read_from_file(infoPath.c_str(), &m_infoPlist, &format) == PLIST_ERR_SUCCESS &&
        m_infoPlist != nullptr) {
        m_udid = getPlistString(m_infoPlist, "Unique Device ID");
        m_deviceName = getPlistString(m_infoPlist, "Device Name");
        m_iosVersion = getPlistString(m_infoPlist, "Product Version");

        plist_t dateNode = plist_dict_get_item(m_infoPlist, "Date");
        if (dateNode) {
            m_backupDate = getPlistString(dateNode, nullptr);
        }

        m_encrypted = getPlistBool(m_infoPlist, "IsEncrypted");
    }
}

BackupFileInfo MobileBackup::parseFileInfo(plist_t fileDict, const std::string& domain) {
    BackupFileInfo info;
    info.domain = domain;
    info.path = getPlistString(fileDict, "Path");
    info.hash = getPlistString(fileDict, "Hash");
    info.size = getPlistUInt64(fileDict, "Size");
    info.isEncrypted = getPlistBool(fileDict, "EncryptionKey");
    info.encryptionKey = getPlistString(fileDict, "EncryptionKey");
    info.mtime = getPlistUInt64(fileDict, "ModificationTime");
    info.ctime = getPlistUInt64(fileDict, "CreationTime");
    info.hasMetadata = !info.path.empty();
    
    return info;
}

std::string MobileBackup::getPlistString(plist_t node, const char* key) const {
    if (!node) {
        return "";
    }
    
    plist_t targetNode = node;
    if (key) {
        targetNode = plist_dict_get_item(node, key);
        if (!targetNode) {
            return "";
        }
    }
    
    char* value = nullptr;
    plist_get_string_val(targetNode, &value);
    
    if (value) {
        std::string result(value);
        free(value);
        return result;
    }
    
    return "";
}

uint64_t MobileBackup::getPlistUInt64(plist_t node, const char* key) const {
    if (!node) {
        return 0;
    }
    
    plist_t targetNode = node;
    if (key) {
        targetNode = plist_dict_get_item(node, key);
        if (!targetNode) {
            return 0;
        }
    }
    
    uint64_t value = 0;
    plist_get_uint_val(targetNode, &value);
    return value;
}

bool MobileBackup::getPlistBool(plist_t node, const char* key) const {
    if (!node) {
        return false;
    }
    
    plist_t targetNode = node;
    if (key) {
        targetNode = plist_dict_get_item(node, key);
        if (!targetNode) {
            return false;
        }
    }
    
    uint8_t value = 0;
    plist_get_bool_val(targetNode, &value);
    return value != 0;
}

std::string MobileBackup::getHashPath(const std::string& hash) const {
    return resolveBackupHashPath(m_backupPath, hash, m_storageProtocol);
}

bool MobileBackup::decryptFile(const std::string& hash, const std::string& key, const std::string& outputPath) const {
    (void)hash;
    (void)key;
    (void)outputPath;
    Logger::warn("File decryption not fully implemented - requires backup password");
    return false;
}

} /* namespace PP */
