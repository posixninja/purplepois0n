/*
 * MbdbParser.cpp
 */

#include "MbdbParser.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace PP {

namespace {

constexpr uint8_t kMbdbHeaderSize = 6;
constexpr uint16_t kAbsentField = 0xFFFF;

bool isMbdbHeader(const uint8_t* data, size_t size) {
    return data != nullptr && size >= kMbdbHeaderSize &&
           data[0] == 'm' && data[1] == 'b' && data[2] == 'd' && data[3] == 'b' &&
           data[5] == 0x00;
}

} /* anonymous namespace */

bool MbdbParser::isMbdbMagic(const uint8_t* data, size_t size) {
    if (!isMbdbHeader(data, size)) {
        return false;
    }
    const uint16_t version = readFormatVersion(data, size);
    return version == 1 || version == 5;
}

uint16_t MbdbParser::readFormatVersion(const uint8_t* data, size_t size) {
    if (!isMbdbHeader(data, size)) {
        return 0;
    }
    return static_cast<uint16_t>(data[4]);
}

uint16_t MbdbParser::readBe16(const uint8_t* p) {
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

uint32_t MbdbParser::readBe32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

uint64_t MbdbParser::readBe64(const uint8_t* p) {
    return (static_cast<uint64_t>(readBe32(p)) << 32) | readBe32(p + 4);
}

bool MbdbParser::readStringField(const uint8_t* data, size_t size, size_t& offset,
                                 std::string& out, bool allowAbsentMarker) {
    if (offset + 2 > size) {
        return false;
    }
    const uint16_t length = readBe16(data + offset);
    offset += 2;

    if (length == 0) {
        out.clear();
        return true;
    }
    if (allowAbsentMarker && length == kAbsentField) {
        out.clear();
        return true;
    }
    if (offset + length > size) {
        return false;
    }
    out.assign(reinterpret_cast<const char*>(data + offset), length);
    offset += length;
    return true;
}

bool MbdbParser::parseRecord(const uint8_t* data, size_t size, size_t& offset,
                             MbdbRecord& out) {
    const size_t start = offset;

    if (!readStringField(data, size, offset, out.domain, false)) {
        return false;
    }
    if (!readStringField(data, size, offset, out.path, false)) {
        return false;
    }
    if (!readStringField(data, size, offset, out.target, true)) {
        return false;
    }
    if (!readStringField(data, size, offset, out.dataHash, true)) {
        return false;
    }

    std::string unknown1;
    if (!readStringField(data, size, offset, unknown1, true)) {
        return false;
    }

    if (offset + 2 > size) {
        return false;
    }
    out.mode = readBe16(data + offset);
    offset += 2;

    if (offset + 4 > size) {
        return false;
    }
    offset += 4; /* unknown2 */

    if (offset + 28 > size) {
        return false;
    }
    out.inode = readBe32(data + offset);
    offset += 4;
    out.uid = readBe32(data + offset);
    offset += 4;
    out.gid = readBe32(data + offset);
    offset += 4;
    out.mtime = readBe32(data + offset);
    offset += 4;
    out.ctime = readBe32(data + offset);
    offset += 4;
    out.atime = readBe32(data + offset);
    offset += 4;
    out.size = readBe64(data + offset);
    offset += 8;

    if (offset + 2 > size) {
        return false;
    }
    out.flag = data[offset++];
    out.propertyCount = data[offset++];

    for (uint8_t i = 0; i < out.propertyCount; ++i) {
        std::string propName;
        std::string propValue;
        if (!readStringField(data, size, offset, propName, false)) {
            return false;
        }
        if (!readStringField(data, size, offset, propValue, false)) {
            return false;
        }
    }

    if (offset <= start) {
        return false;
    }
    return true;
}

bool MbdbParser::parse(const std::vector<uint8_t>& data) {
    m_records.clear();
    m_formatVersion = 0;
    if (!isMbdbHeader(data.data(), data.size())) {
        return false;
    }

    m_formatVersion = readFormatVersion(data.data(), data.size());
    if (m_formatVersion != 1 && m_formatVersion != 5) {
        return false;
    }

    size_t offset = kMbdbHeaderSize;
    while (offset < data.size()) {
        MbdbRecord record;
        const size_t before = offset;
        if (!parseRecord(data.data(), data.size(), offset, record)) {
            break;
        }
        if (offset == before) {
            break;
        }
        m_records.push_back(record);
    }

    return !m_records.empty();
}

bool MbdbParser::parseFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());
    return parse(data);
}

std::string MbdbParser::hashToHex(const std::string& binaryHash) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < binaryHash.size(); ++i) {
        oss << std::setw(2) << (static_cast<unsigned>(binaryHash[i]) & 0xFFu);
    }
    return oss.str();
}

} /* namespace PP */
