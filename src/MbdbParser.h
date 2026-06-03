/*
 * MbdbParser.h
 *
 * Clean-room parser for iOS Manifest.mbdb (absinthe / iOS 5–9 era).
 * Format reference: Apple mobile backup manifest v5; study paths in docs/legacy/.
 */

#ifndef MBDB_PARSER_H_
#define MBDB_PARSER_H_

#include <cstdint>
#include <string>
#include <vector>

namespace PP {

/** One entry from Manifest.mbdb. */
struct MbdbRecord {
    std::string domain;
    std::string path;
    std::string target;
    std::string dataHash;       ///< 20-byte SHA1, may be empty for dirs/links
    uint16_t mode = 0;
    uint32_t inode = 0;
    uint32_t uid = 0;
    uint32_t gid = 0;
    uint32_t mtime = 0;
    uint32_t ctime = 0;
    uint32_t atime = 0;
    uint64_t size = 0;
    uint8_t flag = 0;
    uint8_t propertyCount = 0;
};

/** Parsed Manifest.mbdb file. */
class MbdbParser {
public:
    /** Parse manifest bytes; returns false on invalid magic or corrupt records. */
    bool parse(const std::vector<uint8_t>& data);

    /** Load and parse a Manifest.mbdb path. */
    bool parseFile(const std::string& path);

    const std::vector<MbdbRecord>& records() const { return m_records; }

    /** mbdb format version from header (typically 5). */
    uint16_t formatVersion() const { return m_formatVersion; }

    /** True for mbdb\\x05\\x00 and other supported header versions. */
    static bool isMbdbMagic(const uint8_t* data, size_t size);

    /** Read mbdb major version from magic bytes 4–5; returns 0 when invalid. */
    static uint16_t readFormatVersion(const uint8_t* data, size_t size);

    /** Hex-encode a binary SHA1 hash for backup directory layout. */
    static std::string hashToHex(const std::string& binaryHash);

private:
    bool parseRecord(const uint8_t* data, size_t size, size_t& offset, MbdbRecord& out);

    static uint16_t readBe16(const uint8_t* p);
    static uint32_t readBe32(const uint8_t* p);
    static uint64_t readBe64(const uint8_t* p);
    static bool readStringField(const uint8_t* data, size_t size, size_t& offset,
                                std::string& out, bool allowAbsentMarker);

    std::vector<MbdbRecord> m_records;
    uint16_t m_formatVersion = 0;
};

} /* namespace PP */

#endif /* MBDB_PARSER_H_ */
