/*
 * RamdiskStager.cpp
 */

#include "RamdiskStager.h"
#include "RamdiskBuilder.h"
#include "MachOBinary.h"
#include "Logger.h"

#include <fstream>
#include <sys/stat.h>

namespace PP {

namespace {

bool readHostFile(const std::string& path, std::vector<uint8_t>* data) {
    if (data == nullptr) {
        return false;
    }
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    data->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool looksLikeMachO(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return false;
    }
    const uint32_t magic =
        (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
    return magic == 0xFEEDFACF || magic == 0xCFFAEDFE || magic == 0xFEEDFACE ||
           magic == 0xCEFAEDFE;
}

uint16_t modeFromHostFile(const std::string& path, bool executableHint) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return executableHint ? static_cast<uint16_t>(0100755) : static_cast<uint16_t>(0100644);
    }
    if (executableHint || (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0) {
        return static_cast<uint16_t>(0100755);
    }
    return static_cast<uint16_t>(0100644);
}

} /* anonymous */

bool stageHostFile(RamdiskBuilder* builder, const RamdiskStageEntry& entry) {
    if (builder == nullptr || entry.hostPath.empty() || entry.hfsPath.empty()) {
        return false;
    }
    std::vector<uint8_t> data;
    if (!readHostFile(entry.hostPath, &data)) {
        Logger::error("  [Ramdisk] cannot read host file: " + entry.hostPath);
        return false;
    }

    uint16_t mode = entry.mode;
    const bool machoCandidate = entry.verifyMachOArm64 || looksLikeMachO(data);
    if (machoCandidate) {
        const std::unique_ptr<MachOBinary> macho =
            MachOBinary::open(entry.hostPath, MachOArchPreference::Arm64);
        if (macho == nullptr || !macho->isValid()) {
            if (entry.verifyMachOArm64) {
                Logger::error("  [Ramdisk] --ramdisk-add-macho requires arm64 Mach-O: " +
                              entry.hostPath);
                return false;
            }
            Logger::warn("  [Ramdisk] not arm64 Mach-O (staging anyway): " + entry.hostPath);
        } else {
            const std::string arch = macho->architectureName();
            if (arch.find("arm64") == std::string::npos && arch.find("ARM64") == std::string::npos) {
                if (entry.verifyMachOArm64) {
                    Logger::error("  [Ramdisk] expected arm64 slice in: " + entry.hostPath +
                                  " (got " + arch + ")");
                    return false;
                }
                Logger::warn("  [Ramdisk] Mach-O arch is " + arch + " — iOS ramdisk expects arm64");
            } else {
                Logger::info("  [Ramdisk] staged arm64 Mach-O from Mac → " + entry.hfsPath +
                             " (" + arch + ")");
                Logger::info("  [Ramdisk] note: macOS arm64 ≠ iOS — cross-compile or use iOS-built binary");
            }
            mode = modeFromHostFile(entry.hostPath, true);
        }
    } else {
        mode = modeFromHostFile(entry.hostPath, false);
    }

    if (!builder->addFile(entry.hfsPath, data, mode)) {
        Logger::error("  [Ramdisk] failed to add: " + entry.hfsPath);
        return false;
    }
    Logger::info("  [Ramdisk] staged " + entry.hostPath + " → " + entry.hfsPath);
    return true;
}

bool stageHostFiles(RamdiskBuilder* builder, const std::vector<RamdiskStageEntry>& entries) {
    for (size_t i = 0; i < entries.size(); ++i) {
        if (!stageHostFile(builder, entries[i])) {
            return false;
        }
    }
    return true;
}

} /* namespace PP */
