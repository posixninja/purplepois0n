/*
 * MachOParser.cpp
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#include "MachOParser.h"
#include <algorithm>
#include <cstring>
#include "Logger.h"

namespace PP {

namespace {

constexpr uint32_t kLcSegment = 0x1;
constexpr uint32_t kLcSegment64 = 0x19;
constexpr uint32_t kLcSymtab = 0x2;
constexpr uint32_t kLcMain = 0x28;
constexpr uint32_t kLcDyldInfo = 0x22;
constexpr uint32_t kLcDyldInfoOnly = 0x80000022;

constexpr uint32_t kMagicFat = 0xCAFEBABE;
constexpr uint32_t kMagicFat64 = 0xCAFEBABF;
constexpr uint32_t kMagicMach32 = 0xFEEDFACE;
constexpr uint32_t kMagicMach64 = 0xFEEDFACF;
constexpr uint32_t kMagicMach32Le = 0xCEFAEDFE;
constexpr uint32_t kMagicMach64Le = 0xCFFAEDFE;

bool isThinMachMagic(uint32_t magic, bool& is64) {
    if (magic == kMagicMach32 || magic == kMagicMach32Le) {
        is64 = false;
        return true;
    }
    if (magic == kMagicMach64 || magic == kMagicMach64Le) {
        is64 = true;
        return true;
    }
    return false;
}

uint32_t readMagicAtOffset(std::ifstream& file, uint64_t offset) {
    file.seekg(static_cast<std::streamoff>(offset));
    uint8_t bytes[4] = {0};
    file.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
    const uint32_t be = (static_cast<uint32_t>(bytes[0]) << 24) |
                        (static_cast<uint32_t>(bytes[1]) << 16) |
                        (static_cast<uint32_t>(bytes[2]) << 8) |
                        static_cast<uint32_t>(bytes[3]);
    const uint32_t le = static_cast<uint32_t>(bytes[0]) |
                        (static_cast<uint32_t>(bytes[1]) << 8) |
                        (static_cast<uint32_t>(bytes[2]) << 16) |
                        (static_cast<uint32_t>(bytes[3]) << 24);
    bool is64 = false;
    if (isThinMachMagic(be, is64) || be == kMagicFat || be == kMagicFat64) {
        return be;
    }
    if (isThinMachMagic(le, is64)) {
        return le;
    }
    return be;
}

constexpr uint32_t kCpuTypeArm32 = 0x0000000C;
constexpr uint32_t kCpuTypeArm64 = 0x0100000C;

} /* anonymous namespace */

MachOArchPreference machOArchPreferenceFromString(const std::string& arch) {
    if (arch == "arm32" || arch == "armv7" || arch == "armv6" || arch == "32") {
        return MachOArchPreference::Arm32;
    }
    if (arch == "arm64" || arch == "arm64e" || arch == "64") {
        return MachOArchPreference::Arm64;
    }
    return MachOArchPreference::Default;
}

std::string machOArchPreferenceName(MachOArchPreference preference) {
    switch (preference) {
        case MachOArchPreference::Arm32:
            return "arm32";
        case MachOArchPreference::Arm64:
            return "arm64";
        default:
            return "default";
    }
}

MachOParser::MachOParser(const std::string& binaryPath, MachOArchPreference archPreference)
    : m_binaryPath(binaryPath), m_archPreference(archPreference), m_valid(false), m_is64Bit(false),
      m_cpuType(MachOCpuType::ARM), m_fileType(MachOFileType::EXECUTE),
      m_ncmds(0), m_sizeofcmds(0), m_headerOffset(0), m_entryPoint(0) {
    m_file.open(binaryPath, std::ios::binary);
    if (!m_file.is_open()) {
        throw std::runtime_error("Failed to open Mach-O file: " + binaryPath);
    }

    parseHeader();
    if (m_valid) {
        parseLoadCommands();
        if (m_symtab.present) {
            loadSymbols();
        }
    }
}

MachOParser::~MachOParser() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

bool MachOParser::isValid() const {
    return m_valid;
}

bool MachOParser::is64Bit() const {
    return m_is64Bit;
}

MachOCpuType MachOParser::getCpuType() const {
    return m_cpuType;
}

std::string MachOParser::getArchitectureName() const {
    switch (m_cpuType) {
        case MachOCpuType::ARM:
            return "arm32";
        case MachOCpuType::ARM64:
            return "arm64";
        case MachOCpuType::X86:
            return "x86";
        case MachOCpuType::X86_64:
            return "x86_64";
        default:
            return "unknown";
    }
}

bool MachOParser::isArm32() const {
    return m_cpuType == MachOCpuType::ARM && !m_is64Bit;
}

bool MachOParser::isArm64() const {
    return m_cpuType == MachOCpuType::ARM64 && m_is64Bit;
}

MachOArchPreference MachOParser::getArchPreference() const {
    return m_archPreference;
}

MachOFileType MachOParser::getFileType() const {
    return m_fileType;
}

uint32_t MachOParser::getLoadCommandCount() const {
    return m_ncmds;
}

std::vector<MachOSegment> MachOParser::getSegments() const {
    return m_segments;
}

std::unique_ptr<MachOSegment> MachOParser::findSegment(const std::string& segname) const {
    for (const auto& seg : m_segments) {
        if (seg.segname == segname) {
            auto segment = std::make_unique<MachOSegment>();
            *segment = seg;
            return segment;
        }
    }
    return nullptr;
}

std::vector<MachOSection> MachOParser::getSections() const {
    return m_sections;
}

std::unique_ptr<MachOSection> MachOParser::findSection(const std::string& segname, const std::string& sectname) const {
    for (const auto& sect : m_sections) {
        if (sect.segname == segname && sect.sectname == sectname) {
            auto section = std::make_unique<MachOSection>();
            *section = sect;
            return section;
        }
    }
    return nullptr;
}

std::vector<MachOLoadCommand> MachOParser::getLoadCommands() const {
    return m_loadCommands;
}

uint64_t MachOParser::getEntryPoint() const {
    return m_entryPoint;
}

std::vector<uint8_t> MachOParser::extractSection(const std::string& segname, const std::string& sectname) const {
    auto section = findSection(segname, sectname);
    if (!section) {
        Logger::error("Section not found: " + segname + "," + sectname);
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> data(section->size);
    m_file.seekg(section->offset);
    m_file.read(reinterpret_cast<char*>(data.data()), section->size);
    
    return data;
}

MachOSymtabInfo MachOParser::getSymtabInfo() const {
    return m_symtab;
}

MachODyldInfo MachOParser::getDyldInfo() const {
    return m_dyldInfo;
}

std::vector<MachOSymbol> MachOParser::getSymbols() const {
    return m_symbols;
}

uint64_t MachOParser::fileBaseOffset() const {
    return m_headerOffset;
}

uint64_t MachOParser::computeMappedFileSize(const std::string& path, uint64_t offset) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return 0;
    }

    file.seekg(static_cast<std::streamoff>(offset));
    if (!file) {
        return 0;
    }

    const uint32_t magic = readMagicAtOffset(file, offset);

    bool is64 = false;
    if (!isThinMachMagic(magic, is64)) {
        return 0;
    }

    uint32_t ncmds = 0;
    file.seekg(static_cast<std::streamoff>(offset + (is64 ? 16 : 12)));
    file.read(reinterpret_cast<char*>(&ncmds), sizeof(ncmds));

    uint64_t cmdOffset = offset + (is64 ? 32u : 28u);
    uint64_t end = offset;

    for (uint32_t i = 0; i < ncmds; ++i) {
        file.seekg(static_cast<std::streamoff>(cmdOffset));
        uint32_t cmd = 0;
        uint32_t cmdsize = 0;
        file.read(reinterpret_cast<char*>(&cmd), sizeof(cmd));
        file.read(reinterpret_cast<char*>(&cmdsize), sizeof(cmdsize));
        if (!file || cmdsize < 8) {
            break;
        }

        if (cmd == kLcSegment64 && is64) {
            file.seekg(static_cast<std::streamoff>(cmdOffset + 48));
            uint64_t fileoff = 0;
            uint64_t filesize = 0;
            file.read(reinterpret_cast<char*>(&fileoff), sizeof(fileoff));
            file.read(reinterpret_cast<char*>(&filesize), sizeof(filesize));
            end = std::max(end, offset + fileoff + filesize);
        } else if (cmd == kLcSegment && !is64) {
            file.seekg(static_cast<std::streamoff>(cmdOffset + 24));
            uint32_t fileoff = 0;
            uint32_t filesize = 0;
            file.read(reinterpret_cast<char*>(&fileoff), sizeof(fileoff));
            file.read(reinterpret_cast<char*>(&filesize), sizeof(filesize));
            end = std::max(end, offset + fileoff + filesize);
        } else if (cmd == kLcSymtab) {
            file.seekg(static_cast<std::streamoff>(cmdOffset + 8));
            uint32_t symoff = 0;
            uint32_t nsyms = 0;
            uint32_t stroff = 0;
            uint32_t strsize = 0;
            file.read(reinterpret_cast<char*>(&symoff), sizeof(symoff));
            file.read(reinterpret_cast<char*>(&nsyms), sizeof(nsyms));
            file.read(reinterpret_cast<char*>(&stroff), sizeof(stroff));
            file.read(reinterpret_cast<char*>(&strsize), sizeof(strsize));
            const uint64_t symEntrySize = is64 ? 16u : 12u;
            end = std::max(end, offset + symoff + static_cast<uint64_t>(nsyms) * symEntrySize);
            end = std::max(end, offset + stroff + strsize);
        }

        cmdOffset += cmdsize;
    }

    return end > offset ? end - offset : 0;
}

bool MachOParser::cpuTypeMatchesPreference(uint32_t cputype) const {
    if (m_archPreference == MachOArchPreference::Default) {
        return true;
    }
    if (m_archPreference == MachOArchPreference::Arm32) {
        return cputype == kCpuTypeArm32;
    }
    if (m_archPreference == MachOArchPreference::Arm64) {
        return cputype == kCpuTypeArm64;
    }
    return false;
}

bool MachOParser::selectFatSlice(uint32_t fatMagic) {
    const uint32_t nfat = readUInt32BE(m_headerOffset + 4);
    const bool fat64 = (fatMagic == kMagicFat64);
    const uint64_t archStride = fat64 ? 32u : 20u;

    uint64_t fallbackOffset = 0;
    bool haveFallback = false;

    for (uint32_t i = 0; i < nfat; ++i) {
        const uint64_t archBase = m_headerOffset + 8 + static_cast<uint64_t>(i) * archStride;
        const uint32_t cputype = readUInt32BE(archBase);
        const uint64_t sliceOffset =
            fat64 ? readUInt64BE(archBase + 16) : readUInt32BE(archBase + 8);

        if (!haveFallback) {
            fallbackOffset = sliceOffset;
            haveFallback = true;
        }
        if (cpuTypeMatchesPreference(cputype)) {
            m_headerOffset = sliceOffset;
            return true;
        }
    }

    if (haveFallback) {
        if (m_archPreference != MachOArchPreference::Default) {
            Logger::warn("No fat slice matched arch preference — using first slice");
        }
        m_headerOffset = fallbackOffset;
        return true;
    }
    return false;
}

void MachOParser::parseHeader() {
    m_file.seekg(0);

    uint32_t magic = readMagicBE(m_headerOffset);
    if (magic == kMagicFat || magic == kMagicFat64) {
        Logger::warn("Fat binary detected - selecting architecture slice");
        if (!selectFatSlice(magic)) {
            m_valid = false;
            return;
        }
        magic = readMagicAtOffset(m_file, m_headerOffset);
    }

    bool is64 = false;
    if (isThinMachMagic(magic, is64)) {
        m_is64Bit = is64;
        if (m_is64Bit) {
            MachOHeader64 header;
            m_file.seekg(m_headerOffset);
            m_file.read(reinterpret_cast<char*>(&header), sizeof(MachOHeader64));

            m_cpuType = static_cast<MachOCpuType>(header.cputype);
            m_fileType = static_cast<MachOFileType>(header.filetype);
            m_ncmds = header.ncmds;
            m_sizeofcmds = header.sizeofcmds;
            m_valid = true;
        } else {
            MachOHeader32 header;
            m_file.seekg(m_headerOffset);
            m_file.read(reinterpret_cast<char*>(&header), sizeof(MachOHeader32));

            m_cpuType = static_cast<MachOCpuType>(header.cputype);
            m_fileType = static_cast<MachOFileType>(header.filetype);
            m_ncmds = header.ncmds;
            m_sizeofcmds = header.sizeofcmds;
            m_valid = true;
        }
    } else {
        Logger::error("Invalid Mach-O magic: 0x" + std::to_string(magic));
        m_valid = false;
    }
}

void MachOParser::parseLoadCommands() {
    uint64_t offset = m_headerOffset + (m_is64Bit ? sizeof(MachOHeader64) : sizeof(MachOHeader32));
    
    for (uint32_t i = 0; i < m_ncmds; i++) {
        uint32_t cmd = readUInt32(offset);
        uint32_t cmdsize = readUInt32(offset + sizeof(uint32_t));
        
        MachOLoadCommand lc;
        lc.cmd = cmd;
        lc.cmdsize = cmdsize;
        lc.data.resize(cmdsize);
        
        m_file.seekg(offset);
        m_file.read(reinterpret_cast<char*>(lc.data.data()), cmdsize);
        
        m_loadCommands.push_back(lc);
        
        // Parse specific commands
        if (cmd == kLcSegment64) {
            parseSegment(cmd, cmdsize, offset);
        } else if (cmd == kLcSegment) {
            parseSegment(cmd, cmdsize, offset);
        } else if (cmd == kLcMain) {
            if (m_is64Bit) {
                m_entryPoint = readUInt64(offset + 8);
            }
        } else if (cmd == kLcSymtab) {
            parseSymtab(offset);
        } else if (cmd == kLcDyldInfo || cmd == kLcDyldInfoOnly) {
            parseDyldInfo(offset);
        }
        
        offset += cmdsize;
    }
}

void MachOParser::parseSegment(uint32_t cmd, uint32_t cmdsize, uint64_t offset) {
    MachOSegment seg;
    
    if (m_is64Bit && cmd == 0x19) {
        SegmentCommand64 segCmd;
        m_file.seekg(offset);
        m_file.read(reinterpret_cast<char*>(&segCmd), sizeof(SegmentCommand64));
        
        seg.segname = readString(offset + 8, 16);
        seg.vmaddr = segCmd.vmaddr;
        seg.vmsize = segCmd.vmsize;
        seg.fileoff = segCmd.fileoff;
        seg.filesize = segCmd.filesize;
        seg.maxprot = segCmd.maxprot;
        seg.initprot = segCmd.initprot;
        seg.nsects = segCmd.nsects;
        seg.flags = segCmd.flags;
        
        // Parse sections
        uint64_t sectOffset = offset + sizeof(SegmentCommand64);
        for (uint32_t i = 0; i < seg.nsects; i++) {
            parseSection(sectOffset, seg.segname);
            sectOffset += 80; // Size of section_64
        }
    } else if (!m_is64Bit && cmd == 0x1) {
        SegmentCommand32 segCmd;
        m_file.seekg(offset);
        m_file.read(reinterpret_cast<char*>(&segCmd), sizeof(SegmentCommand32));
        
        seg.segname = readString(offset + 8, 16);
        seg.vmaddr = segCmd.vmaddr;
        seg.vmsize = segCmd.vmsize;
        seg.fileoff = segCmd.fileoff;
        seg.filesize = segCmd.filesize;
        seg.maxprot = segCmd.maxprot;
        seg.initprot = segCmd.initprot;
        seg.nsects = segCmd.nsects;
        seg.flags = segCmd.flags;
        
        // Parse sections
        uint64_t sectOffset = offset + sizeof(SegmentCommand32);
        for (uint32_t i = 0; i < seg.nsects; i++) {
            parseSection(sectOffset, seg.segname);
            sectOffset += 68; // Size of section
        }
    }
    
    m_segments.push_back(seg);
}

void MachOParser::parseSection(uint64_t offset, const std::string& segname) {
    MachOSection sect;
    sect.segname = segname;
    
    if (m_is64Bit) {
        sect.sectname = readString(offset, 16);
        sect.addr = readUInt64(offset + 16);
        sect.size = readUInt64(offset + 24);
        sect.offset = readUInt32(offset + 32);
        sect.align = readUInt32(offset + 36);
        sect.reloff = readUInt32(offset + 40);
        sect.nreloc = readUInt32(offset + 44);
        sect.flags = readUInt32(offset + 48);
        sect.reserved1 = readUInt32(offset + 52);
        sect.reserved2 = readUInt32(offset + 56);
    } else {
        sect.sectname = readString(offset, 16);
        sect.addr = readUInt32(offset + 16);
        sect.size = readUInt32(offset + 20);
        sect.offset = readUInt32(offset + 24);
        sect.align = readUInt32(offset + 28);
        sect.reloff = readUInt32(offset + 32);
        sect.nreloc = readUInt32(offset + 36);
        sect.flags = readUInt32(offset + 40);
        sect.reserved1 = readUInt32(offset + 44);
        sect.reserved2 = readUInt32(offset + 48);
    }
    
    m_sections.push_back(sect);
}

void MachOParser::parseSymtab(uint64_t offset) {
    m_symtab.present = true;
    m_symtab.symoff = readUInt32(offset + 8);
    m_symtab.nsyms = readUInt32(offset + 12);
    m_symtab.stroff = readUInt32(offset + 16);
    m_symtab.strsize = readUInt32(offset + 20);
}

void MachOParser::parseDyldInfo(uint64_t offset) {
    m_dyldInfo.present = true;
    m_dyldInfo.rebaseOffset = readUInt32(offset + 8);
    m_dyldInfo.rebaseSize = readUInt32(offset + 12);
    m_dyldInfo.bindOffset = readUInt32(offset + 16);
    m_dyldInfo.bindSize = readUInt32(offset + 20);
    m_dyldInfo.weakBindOffset = readUInt32(offset + 24);
    m_dyldInfo.weakBindSize = readUInt32(offset + 28);
    m_dyldInfo.lazyBindOffset = readUInt32(offset + 32);
    m_dyldInfo.lazyBindSize = readUInt32(offset + 36);
    m_dyldInfo.exportOffset = readUInt32(offset + 40);
    m_dyldInfo.exportSize = readUInt32(offset + 44);
}

void MachOParser::loadSymbols() {
    m_symbols.clear();
    if (!m_symtab.present || m_symtab.nsyms == 0) {
        return;
    }

    const uint64_t base = fileBaseOffset();
    const uint64_t symEntrySize = m_is64Bit ? 16u : 12u;

    for (uint32_t i = 0; i < m_symtab.nsyms; ++i) {
        const uint64_t entryOffset = base + m_symtab.symoff + static_cast<uint64_t>(i) * symEntrySize;
        MachOSymbol sym;

        uint32_t nStrx = readUInt32(entryOffset);
        const uint32_t meta = readUInt32(entryOffset + 4);
        sym.type = static_cast<uint8_t>(meta & 0xFF);
        sym.sect = static_cast<uint8_t>((meta >> 8) & 0xFF);
        sym.desc = static_cast<uint16_t>((meta >> 16) & 0xFFFF);

        if (m_is64Bit) {
            sym.value = readUInt64(entryOffset + 8);
        } else {
            sym.value = readUInt32(entryOffset + 8);
        }

        if (nStrx < m_symtab.strsize) {
            sym.name = readString(base + m_symtab.stroff + nStrx, m_symtab.strsize - nStrx);
        }

        if (!sym.name.empty()) {
            m_symbols.push_back(sym);
        }
    }
}

uint32_t MachOParser::readUInt32BE(uint64_t offset) const {
    return readMagicBE(offset);
}

uint64_t MachOParser::readUInt64BE(uint64_t offset) const {
    std::streampos oldPos = m_file.tellg();
    m_file.seekg(static_cast<std::streamoff>(offset));

    uint8_t bytes[8] = {0};
    m_file.read(reinterpret_cast<char*>(bytes), sizeof(bytes));

    m_file.seekg(oldPos);
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | static_cast<uint64_t>(bytes[i]);
    }
    return value;
}

uint32_t MachOParser::readMagicBE(uint64_t offset) const {
    std::streampos oldPos = m_file.tellg();
    m_file.seekg(static_cast<std::streamoff>(offset));

    uint8_t bytes[4] = {0};
    m_file.read(reinterpret_cast<char*>(bytes), sizeof(bytes));

    m_file.seekg(oldPos);
    return (static_cast<uint32_t>(bytes[0]) << 24) |
           (static_cast<uint32_t>(bytes[1]) << 16) |
           (static_cast<uint32_t>(bytes[2]) << 8) |
           static_cast<uint32_t>(bytes[3]);
}

uint32_t MachOParser::readUInt32(uint64_t offset) const {
    std::streampos oldPos = m_file.tellg();
    m_file.seekg(offset);
    
    uint32_t value = 0;
    m_file.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
    
    m_file.seekg(oldPos);
    return value;
}

uint64_t MachOParser::readUInt64(uint64_t offset) const {
    std::streampos oldPos = m_file.tellg();
    m_file.seekg(offset);
    
    uint64_t value = 0;
    m_file.read(reinterpret_cast<char*>(&value), sizeof(uint64_t));
    
    m_file.seekg(oldPos);
    return value;
}

std::string MachOParser::readString(uint64_t offset, size_t maxLen) const {
    std::streampos oldPos = m_file.tellg();
    m_file.seekg(offset);
    
    std::string str;
    char c;
    size_t count = 0;
    while (m_file.get(c) && c != '\0' && count < maxLen) {
        str += c;
        count++;
    }
    
    m_file.seekg(oldPos);
    return str;
}

} /* namespace PP */

