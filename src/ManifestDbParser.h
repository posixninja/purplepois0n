/*
 * ManifestDbParser.h
 *
 * SQLite parser for iOS Manifest.db (iOS 10+ backup index).
 * Schema reference: community iTunes backup docs; no restore/decrypt paths.
 */

#ifndef MANIFEST_DB_PARSER_H_
#define MANIFEST_DB_PARSER_H_

#include <cstdint>
#include <string>
#include <vector>

namespace PP {

/** One row from the Files table in Manifest.db. */
struct ManifestDbRecord {
    std::string fileID;         ///< 40-char SHA1 hex (content path under backup/)
    std::string domain;
    std::string relativePath;
    int64_t flags = 0;
    uint64_t size = 0;
    uint64_t mtime = 0;
    uint64_t ctime = 0;
    bool hasFileMetadata = false;
    bool isEncryptedEntry = false;
    std::string encryptionKey;  ///< From file BLOB plist when present
};

/** Read-only Manifest.db indexer. */
class ManifestDbParser {
public:
    bool parseFile(const std::string& path);

    const std::vector<ManifestDbRecord>& records() const { return m_records; }

    /** True when path opens as SQLite with a Files table. */
    static bool isManifestDb(const std::string& path);

private:
    static bool parseFileBlob(const std::vector<uint8_t>& blob, ManifestDbRecord& out);

    std::vector<ManifestDbRecord> m_records;
};

} /* namespace PP */

#endif /* MANIFEST_DB_PARSER_H_ */
