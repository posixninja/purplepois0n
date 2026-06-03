/*
 * DyldCacheParser.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef DYLD_CACHE_PARSER_H_
#define DYLD_CACHE_PARSER_H_

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <cstdint>
#include <stdexcept>

namespace PP {

/**
 * @struct DyldCacheImageInfo
 * @brief Information about a library image in the dyld cache
 */
struct DyldCacheImageInfo {
    uint64_t address;           ///< Address of the image in the cache
    uint64_t modTime;           ///< Modification time
    uint64_t inode;             ///< Inode number
    uint32_t pathFileOffset;    ///< Offset to path string
    uint32_t pad;               ///< Padding
    std::string path;           ///< Path to the library
};

/**
 * @struct DyldCacheMappingInfo
 * @brief Memory mapping information for the dyld cache
 */
struct DyldCacheMappingInfo {
    uint64_t address;           ///< Address in cache
    uint64_t size;              ///< Size of mapping
    uint64_t fileOffset;        ///< Offset in file
    uint32_t maxProt;           ///< Maximum protection
    uint32_t initProt;          ///< Initial protection
};

/**
 * @class DyldCacheParser
 * @brief Parser for dyld shared cache files
 * 
 * The dyld shared cache is a pre-linked collection of system libraries
 * used by iOS to speed up application loading. This parser can extract
 * information about libraries contained in the cache.
 */
class DyldCacheParser {
public:
    /**
     * @brief Construct a DyldCacheParser
     * @param cachePath Path to the dyld cache file
     * @throws std::runtime_error if the file cannot be opened
     */
    explicit DyldCacheParser(const std::string& cachePath);
    
    ~DyldCacheParser();

    /**
     * @brief Check if the cache file is valid
     * @return True if valid, false otherwise
     */
    bool isValid() const;

    /**
     * @brief Get the architecture of the cache
     * @return Architecture string (e.g., "arm64", "armv7")
     */
    std::string getArchitecture() const;

    /**
     * @brief Get the UUID of the cache
     * @return UUID as a string
     */
    std::string getUUID() const;

    /**
     * @brief Get all image information from the cache
     * @return Vector of DyldCacheImageInfo structures
     */
    std::vector<DyldCacheImageInfo> getImageInfos() const;

    /**
     * @brief Find an image by path
     * @param imagePath Path to search for
     * @return Image info if found, nullptr otherwise
     */
    std::unique_ptr<DyldCacheImageInfo> findImage(const std::string& imagePath) const;

    /**
     * @brief Extract a library from the cache
     * @param imagePath Path to the library
     * @param outputPath Path to save the extracted library
     * @return True if successful, false otherwise
     */
    bool extractImage(const std::string& imagePath, const std::string& outputPath) const;

    /**
     * @brief Get the base address of the cache
     * @return Base address
     */
    uint64_t getBaseAddress() const;

    /**
     * @brief Get mapping information
     * @return Vector of mapping info structures
     */
    std::vector<DyldCacheMappingInfo> getMappings() const;

    bool isArm32Cache() const;
    bool isArm64Cache() const;

    /**
     * @return Magic prefix from header
     */
    std::string getMagicString() const;

    /**
     * @brief Whether this cache variant is recognized for image enumeration
     */
    bool isSupportedVariant() const;

private:
    struct CacheHeader {
        char magic[16];
        uint32_t mappingOffset;
        uint32_t mappingCount;
        uint32_t imagesOffset;
        uint32_t imagesCount;
        uint64_t dyldBaseAddress;
        uint64_t codeSignatureOffset;
        uint64_t codeSignatureSize;
        uint64_t slideInfoOffset;
        uint64_t slideInfoSize;
        uint64_t localSymbolsOffset;
        uint64_t localSymbolsSize;
        uint8_t uuid[16];
        uint64_t cacheType;
        uint8_t padding[7];
    };

    void parseHeader();
    void parseMappings();
    void parseImages();
    void detectArchitectureFromMagic();
    std::string readString(uint32_t offset) const;
    uint64_t readUInt64(uint64_t offset) const;
    uint32_t readUInt32(uint64_t offset) const;

    std::string m_cachePath;
    mutable std::ifstream m_file;
    CacheHeader m_header;
    std::vector<DyldCacheMappingInfo> m_mappings;
    std::vector<DyldCacheImageInfo> m_images;
    bool m_valid;
    bool m_supportedVariant;
    std::string m_architecture;
    std::string m_magicString;
};

} /* namespace PP */

#endif /* DYLD_CACHE_PARSER_H_ */

