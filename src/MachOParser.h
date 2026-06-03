/*
 * MachOParser.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef MACHO_PARSER_H_
#define MACHO_PARSER_H_

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <cstdint>
#include <stdexcept>

namespace PP {

/**
 * @enum MachOCpuType
 * @brief CPU type identifiers for Mach-O binaries
 */
enum class MachOCpuType {
    ARM = 0x0000000C,       ///< 32-bit ARM (armv6/armv7)
    ARM64 = 0x0100000C,
    X86 = 0x00000007,
    X86_64 = 0x01000007
};

/** Preferred slice when parsing universal (fat) Mach-O binaries. */
enum class MachOArchPreference {
    Default,  ///< First slice in fat header
    Arm32,    ///< CPU_TYPE_ARM
    Arm64     ///< CPU_TYPE_ARM64
};

/** Parse CLI/API arch string ("arm32", "arm64", "armv7"). */
MachOArchPreference machOArchPreferenceFromString(const std::string& arch);
std::string machOArchPreferenceName(MachOArchPreference preference);

/**
 * @enum MachOFileType
 * @brief File type identifiers for Mach-O binaries
 */
enum class MachOFileType {
    OBJECT = 0x1,
    EXECUTE = 0x2,
    FVMLIB = 0x3,
    CORE = 0x4,
    PRELOAD = 0x5,
    DYLIB = 0x6,
    DYLINKER = 0x7,
    BUNDLE = 0x8,
    DYLIB_STUB = 0x9,
    DSYM = 0xA,
    KEXT_BUNDLE = 0xB
};

/**
 * @struct MachOSegment
 * @brief Information about a segment in a Mach-O binary
 */
struct MachOSegment {
    std::string segname;        ///< Segment name
    uint64_t vmaddr;            ///< Virtual memory address
    uint64_t vmsize;            ///< Virtual memory size
    uint64_t fileoff;           ///< File offset
    uint64_t filesize;          ///< File size
    uint32_t maxprot;           ///< Maximum protection
    uint32_t initprot;          ///< Initial protection
    uint32_t nsects;            ///< Number of sections
    uint32_t flags;             ///< Segment flags
};

/**
 * @struct MachOSection
 * @brief Information about a section in a Mach-O binary
 */
struct MachOSection {
    std::string sectname;       ///< Section name
    std::string segname;         ///< Segment name
    uint64_t addr;              ///< Address
    uint64_t size;              ///< Size
    uint32_t offset;            ///< File offset
    uint32_t align;              ///< Alignment
    uint32_t reloff;            ///< Relocation offset
    uint32_t nreloc;            ///< Number of relocations
    uint32_t flags;             ///< Section flags
    uint32_t reserved1;         ///< Reserved field 1
    uint32_t reserved2;         ///< Reserved field 2
};

/**
 * @struct MachOLoadCommand
 * @brief Information about a load command
 */
struct MachOLoadCommand {
    uint32_t cmd;               ///< Load command type
    uint32_t cmdsize;           ///< Command size
    std::vector<uint8_t> data;  ///< Command data
};

/** LC_SYMTAB metadata (symbol/string table locations). */
struct MachOSymtabInfo {
    bool present = false;
    uint32_t symoff = 0;
    uint32_t nsyms = 0;
    uint32_t stroff = 0;
    uint32_t strsize = 0;
};

/** LC_DYLD_INFO / LC_DYLD_INFO_ONLY bind/rebase/export regions. */
struct MachODyldInfo {
    bool present = false;
    uint32_t rebaseOffset = 0;
    uint32_t rebaseSize = 0;
    uint32_t bindOffset = 0;
    uint32_t bindSize = 0;
    uint32_t weakBindOffset = 0;
    uint32_t weakBindSize = 0;
    uint32_t lazyBindOffset = 0;
    uint32_t lazyBindSize = 0;
    uint32_t exportOffset = 0;
    uint32_t exportSize = 0;
};

/** One nlist / nlist_64 symbol entry. */
struct MachOSymbol {
    std::string name;
    uint64_t value = 0;
    uint8_t type = 0;
    uint8_t sect = 0;
    uint16_t desc = 0;
};

/**
 * @class MachOParser
 * @brief Parser for Mach-O binary files
 * 
 * Mach-O is the native binary format used by macOS and iOS. This parser
 * can extract information about segments, sections, load commands, and
 * other metadata from Mach-O binaries.
 */
class MachOParser {
public:
    /**
     * @brief Construct a MachOParser
     * @param binaryPath Path to the Mach-O binary file
     * @throws std::runtime_error if the file cannot be opened
     */
    explicit MachOParser(const std::string& binaryPath,
                         MachOArchPreference archPreference = MachOArchPreference::Default);
    
    ~MachOParser();

    /**
     * @brief Check if the binary is valid
     * @return True if valid, false otherwise
     */
    bool isValid() const;

    /**
     * @brief Check if the binary is 64-bit
     * @return True if 64-bit, false if 32-bit
     */
    bool is64Bit() const;

    /**
     * @brief Get the CPU type
     * @return CPU type enum value
     */
    MachOCpuType getCpuType() const;

    /** Human-readable arch label: arm32, arm64, x86, x86_64, unknown. */
    std::string getArchitectureName() const;

    bool isArm32() const;
    bool isArm64() const;

    MachOArchPreference getArchPreference() const;

    /**
     * @brief Get the file type
     * @return File type enum value
     */
    MachOFileType getFileType() const;

    /**
     * @brief Get the number of load commands
     * @return Number of load commands
     */
    uint32_t getLoadCommandCount() const;

    /**
     * @brief Get all segments
     * @return Vector of segment structures
     */
    std::vector<MachOSegment> getSegments() const;

    /**
     * @brief Find a segment by name
     * @param segname Segment name to find
     * @return Segment info if found, nullptr otherwise
     */
    std::unique_ptr<MachOSegment> findSegment(const std::string& segname) const;

    /**
     * @brief Get all sections
     * @return Vector of section structures
     */
    std::vector<MachOSection> getSections() const;

    /**
     * @brief Find a section by name
     * @param segname Segment name
     * @param sectname Section name
     * @return Section info if found, nullptr otherwise
     */
    std::unique_ptr<MachOSection> findSection(const std::string& segname, const std::string& sectname) const;

    /**
     * @brief Get all load commands
     * @return Vector of load command structures
     */
    std::vector<MachOLoadCommand> getLoadCommands() const;

    /**
     * @brief Get the entry point address
     * @return Entry point address, or 0 if not found
     */
    uint64_t getEntryPoint() const;

    /**
     * @brief Extract a section's data
     * @param segname Segment name
     * @param sectname Section name
     * @return Vector containing section data
     */
    std::vector<uint8_t> extractSection(const std::string& segname, const std::string& sectname) const;

    /** LC_SYMTAB summary when present. */
    MachOSymtabInfo getSymtabInfo() const;

    /** LC_DYLD_INFO / LC_DYLD_INFO_ONLY summary when present. */
    MachODyldInfo getDyldInfo() const;

    /** Parsed symbol table (empty when no LC_SYMTAB). */
    std::vector<MachOSymbol> getSymbols() const;

    /**
     * Compute on-disk Mach-O size starting at @p offset (segments + linkedit).
     * Used for dyld cache slice extraction.
     */
    static uint64_t computeMappedFileSize(const std::string& path, uint64_t offset = 0);

private:
    struct MachOHeader32 {
        uint32_t magic;
        uint32_t cputype;
        uint32_t cpusubtype;
        uint32_t filetype;
        uint32_t ncmds;
        uint32_t sizeofcmds;
        uint32_t flags;
    };

    struct MachOHeader64 {
        uint32_t magic;
        uint32_t cputype;
        uint32_t cpusubtype;
        uint32_t filetype;
        uint32_t ncmds;
        uint32_t sizeofcmds;
        uint32_t flags;
        uint32_t reserved;
    };

    struct SegmentCommand32 {
        uint32_t cmd;
        uint32_t cmdsize;
        char segname[16];
        uint32_t vmaddr;
        uint32_t vmsize;
        uint32_t fileoff;
        uint32_t filesize;
        uint32_t maxprot;
        uint32_t initprot;
        uint32_t nsects;
        uint32_t flags;
    };

    struct SegmentCommand64 {
        uint32_t cmd;
        uint32_t cmdsize;
        char segname[16];
        uint64_t vmaddr;
        uint64_t vmsize;
        uint64_t fileoff;
        uint64_t filesize;
        uint32_t maxprot;
        uint32_t initprot;
        uint32_t nsects;
        uint32_t flags;
    };

    void parseHeader();
    bool selectFatSlice(uint32_t fatMagic);
    bool cpuTypeMatchesPreference(uint32_t cputype) const;
    void parseLoadCommands();
    void parseSegment(uint32_t cmd, uint32_t cmdsize, uint64_t offset);
    void parseSection(uint64_t offset, const std::string& segname);
    void parseSymtab(uint64_t offset);
    void parseDyldInfo(uint64_t offset);
    void loadSymbols();
    uint64_t fileBaseOffset() const;
    uint32_t readUInt32(uint64_t offset) const;
    uint32_t readUInt32BE(uint64_t offset) const;
    uint32_t readMagicBE(uint64_t offset) const;
    uint64_t readUInt64(uint64_t offset) const;
    uint64_t readUInt64BE(uint64_t offset) const;
    std::string readString(uint64_t offset, size_t maxLen) const;

    std::string m_binaryPath;
    MachOArchPreference m_archPreference;
    mutable std::ifstream m_file;
    bool m_valid;
    bool m_is64Bit;
    MachOCpuType m_cpuType;
    MachOFileType m_fileType;
    uint32_t m_ncmds;
    uint32_t m_sizeofcmds;
    uint64_t m_headerOffset;
    std::vector<MachOSegment> m_segments;
    std::vector<MachOSection> m_sections;
    std::vector<MachOLoadCommand> m_loadCommands;
    uint64_t m_entryPoint;
    MachOSymtabInfo m_symtab;
    MachODyldInfo m_dyldInfo;
    std::vector<MachOSymbol> m_symbols;
};

} /* namespace PP */

#endif /* MACHO_PARSER_H_ */

