/*
 * DyldCacheParser.cpp
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#include "DyldCacheParser.h"
#include "MachOParser.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "Logger.h"

namespace PP {

DyldCacheParser::DyldCacheParser(const std::string& cachePath)
    : m_cachePath(cachePath), m_valid(false), m_supportedVariant(false) {
    m_file.open(cachePath, std::ios::binary);
    if (!m_file.is_open()) {
        throw std::runtime_error("Failed to open dyld cache file: " + cachePath);
    }

    parseHeader();
    if (m_valid) {
        parseMappings();
        parseImages();
    }
}

DyldCacheParser::~DyldCacheParser() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

bool DyldCacheParser::isValid() const {
    return m_valid;
}

bool DyldCacheParser::isArm32Cache() const {
    return m_valid && (m_architecture == "armv6" || m_architecture == "armv7" ||
                       m_architecture == "armv7s" || m_architecture == "armv7k" ||
                       m_architecture == "arm32");
}

bool DyldCacheParser::isArm64Cache() const {
    return m_valid && (m_architecture == "arm64" || m_architecture == "arm64e");
}

std::string DyldCacheParser::getMagicString() const {
    return m_magicString;
}

bool DyldCacheParser::isSupportedVariant() const {
    return m_supportedVariant;
}

std::string DyldCacheParser::getArchitecture() const {
    return m_architecture;
}

std::string DyldCacheParser::getUUID() const {
    if (!m_valid) {
        return "";
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; i++) {
        oss << std::setw(2) << static_cast<unsigned>(m_header.uuid[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            oss << "-";
        }
    }
    return oss.str();
}

std::vector<DyldCacheImageInfo> DyldCacheParser::getImageInfos() const {
    return m_images;
}

std::unique_ptr<DyldCacheImageInfo> DyldCacheParser::findImage(const std::string& imagePath) const {
    for (const auto& image : m_images) {
        if (image.path == imagePath || image.path.find(imagePath) != std::string::npos) {
            auto info = std::make_unique<DyldCacheImageInfo>();
            *info = image;
            return info;
        }
    }
    return nullptr;
}

bool DyldCacheParser::extractImage(const std::string& imagePath, const std::string& outputPath) const {
    auto imageInfo = findImage(imagePath);
    if (!imageInfo) {
        Logger::error("Image not found in cache: " + imagePath);
        return false;
    }

    for (const auto& mapping : m_mappings) {
        if (imageInfo->address >= mapping.address &&
            imageInfo->address < mapping.address + mapping.size) {

            const uint64_t fileOffset = mapping.fileOffset + (imageInfo->address - mapping.address);
            const uint64_t imageSize = MachOParser::computeMappedFileSize(m_cachePath, fileOffset);
            if (imageSize == 0) {
                Logger::error("Could not determine Mach-O size for cache image: " + imagePath);
                return false;
            }

            std::ifstream input(m_cachePath, std::ios::binary);
            std::ofstream output(outputPath, std::ios::binary);
            if (!input.is_open() || !output.is_open()) {
                Logger::error("Failed to open cache/output for image extraction");
                return false;
            }

            input.seekg(static_cast<std::streamoff>(fileOffset));
            std::vector<char> buffer(static_cast<size_t>(imageSize));
            input.read(buffer.data(), static_cast<std::streamsize>(imageSize));
            if (!input) {
                Logger::error("Short read extracting cache image");
                return false;
            }

            output.write(buffer.data(), static_cast<std::streamsize>(imageSize));
            return static_cast<bool>(output);
        }
    }

    Logger::error("No mapping contains image address for: " + imagePath);
    return false;
}

uint64_t DyldCacheParser::getBaseAddress() const {
    return m_valid ? m_header.dyldBaseAddress : 0;
}

std::vector<DyldCacheMappingInfo> DyldCacheParser::getMappings() const {
    return m_mappings;
}

void DyldCacheParser::detectArchitectureFromMagic() {
    m_magicString = std::string(m_header.magic, strnlen(m_header.magic, sizeof(m_header.magic)));
    m_architecture.clear();
    m_supportedVariant = false;

    if (strncmp(m_header.magic, "dyld_v1", 7) == 0 ||
        strncmp(m_header.magic, "dyld_v0", 7) == 0 ||
        strncmp(m_header.magic, "dyld_v2", 7) == 0) {
        m_valid = true;
        const char* archStart = m_header.magic + 7;
        while (*archStart == ' ') {
            ++archStart;
        }
        if (*archStart != '\0') {
            m_architecture = archStart;
        } else if (strncmp(m_header.magic, "dyld_v0", 7) == 0) {
            m_architecture = "armv7";
        } else {
            m_architecture = "arm64";
        }

        if (m_architecture == "armv6" || m_architecture == "armv7" ||
            m_architecture == "armv7s" || m_architecture == "armv7k") {
            m_architecture = "arm32";
        }

        m_supportedVariant = true;
        if (strncmp(m_header.magic, "dyld_v2", 7) == 0) {
            Logger::warn("dyld_v2 cache — using v1-compatible image walk");
        }
        return;
    }

    Logger::warn("Unknown dyld cache magic: " + m_magicString);
    m_valid = false;
}

void DyldCacheParser::parseHeader() {
    m_file.seekg(0);
    m_file.read(reinterpret_cast<char*>(&m_header), sizeof(CacheHeader));

    if (m_file.gcount() != static_cast<std::streamsize>(sizeof(CacheHeader))) {
        Logger::error("Failed to read cache header");
        m_valid = false;
        return;
    }

    detectArchitectureFromMagic();
}

void DyldCacheParser::parseMappings() {
    m_mappings.clear();
    m_file.seekg(m_header.mappingOffset);

    for (uint32_t i = 0; i < m_header.mappingCount; i++) {
        DyldCacheMappingInfo mapping;
        uint64_t currentOffset = m_file.tellg();
        mapping.address = readUInt64(currentOffset);
        currentOffset += sizeof(uint64_t);
        mapping.size = readUInt64(currentOffset);
        currentOffset += sizeof(uint64_t);
        mapping.fileOffset = readUInt64(currentOffset);
        currentOffset += sizeof(uint64_t);
        mapping.maxProt = readUInt32(currentOffset);
        currentOffset += sizeof(uint32_t);
        mapping.initProt = readUInt32(currentOffset);
        m_file.seekg(currentOffset + sizeof(uint32_t));
        m_mappings.push_back(mapping);
    }
}

void DyldCacheParser::parseImages() {
    m_images.clear();
    m_file.seekg(m_header.imagesOffset);

    for (uint32_t i = 0; i < m_header.imagesCount; i++) {
        DyldCacheImageInfo image;
        uint64_t currentOffset = m_file.tellg();
        image.address = readUInt64(currentOffset);
        currentOffset += sizeof(uint64_t);
        image.modTime = readUInt64(currentOffset);
        currentOffset += sizeof(uint64_t);
        image.inode = readUInt64(currentOffset);
        currentOffset += sizeof(uint64_t);
        image.pathFileOffset = readUInt32(currentOffset);
        currentOffset += sizeof(uint32_t);
        image.pad = readUInt32(currentOffset);
        currentOffset += sizeof(uint32_t);
        m_file.seekg(currentOffset);
        
        image.path = readString(image.pathFileOffset);
        m_images.push_back(image);
    }
}

std::string DyldCacheParser::readString(uint32_t offset) const {
    std::streampos oldPos = m_file.tellg();
    m_file.seekg(offset);
    
    std::string str;
    char c;
    while (m_file.get(c) && c != '\0') {
        str += c;
    }
    
    m_file.seekg(oldPos);
    return str;
}

uint64_t DyldCacheParser::readUInt64(uint64_t offset) const {
    std::streampos oldPos = m_file.tellg();
    m_file.seekg(offset);
    
    uint64_t value = 0;
    m_file.read(reinterpret_cast<char*>(&value), sizeof(uint64_t));
    
    m_file.seekg(oldPos);
    return value;
}

uint32_t DyldCacheParser::readUInt32(uint64_t offset) const {
    std::streampos oldPos = m_file.tellg();
    m_file.seekg(offset);
    
    uint32_t value = 0;
    m_file.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
    
    m_file.seekg(oldPos);
    return value;
}

} /* namespace PP */

