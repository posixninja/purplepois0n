/*
 * ManifestDbParser.cpp
 */

#include "ManifestDbParser.h"
#include "KeyedArchiverPlist.h"
#include "Logger.h"
#include <sqlite3.h>

namespace PP {

bool ManifestDbParser::isManifestDb(const std::string& path) {
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (db) {
            sqlite3_close(db);
        }
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='Files'";
    bool ok = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        ok = sqlite3_step(stmt) == SQLITE_ROW;
    }
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return ok;
}

bool ManifestDbParser::parseFileBlob(const std::vector<uint8_t>& blob, ManifestDbRecord& out) {
    const KeyedArchiverFileMetadata meta = KeyedArchiverPlist::parseFileMetadata(blob);
    if (!meta.valid) {
        return false;
    }

    out.hasFileMetadata = true;
    out.size = meta.size;
    out.mtime = meta.mtime;
    out.ctime = meta.ctime;
    out.isEncryptedEntry = meta.hasEncryptionKey;
    out.encryptionKey = meta.encryptionKey;
    return true;
}

bool ManifestDbParser::parseFile(const std::string& path) {
    m_records.clear();

    sqlite3* db = nullptr;
    if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        Logger::warn("Failed to open Manifest.db: " + path);
        if (db) {
            sqlite3_close(db);
        }
        return false;
    }

    const char* sql = "SELECT fileID, domain, relativePath, flags, file FROM Files";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::warn("Manifest.db missing Files table");
        sqlite3_close(db);
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ManifestDbRecord record;
        const unsigned char* fileID = sqlite3_column_text(stmt, 0);
        const unsigned char* domain = sqlite3_column_text(stmt, 1);
        const unsigned char* relativePath = sqlite3_column_text(stmt, 2);

        if (fileID) {
            record.fileID = reinterpret_cast<const char*>(fileID);
        }
        if (domain) {
            record.domain = reinterpret_cast<const char*>(domain);
        }
        if (relativePath) {
            record.relativePath = reinterpret_cast<const char*>(relativePath);
        }
        record.flags = sqlite3_column_int64(stmt, 3);

        const void* blob = sqlite3_column_blob(stmt, 4);
        int blobLen = sqlite3_column_bytes(stmt, 4);
        if (blob && blobLen > 0) {
            const uint8_t* bytes = static_cast<const uint8_t*>(blob);
            std::vector<uint8_t> blobData(bytes, bytes + blobLen);
            parseFileBlob(blobData, record);
        }

        m_records.push_back(record);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return !m_records.empty();
}

} /* namespace PP */
