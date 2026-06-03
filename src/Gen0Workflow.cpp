/*
 * Gen0Workflow.cpp
 */

#include "Gen0Workflow.h"
#include "DeviceManager.h"
#include "Logger.h"
#include "MobileBackup.h"
#include "BackupProtocol.h"
#include "MachOBinary.h"
#include "DyldSharedCache.h"
#include "DFUDevice.h"
#include "RecoveryDevice.h"
#include "MobileDevice.h"
#include "Checkm8.h"
#include "primitives/ChainRunner.h"
#include "primitives/DfuTransport.h"
#include "primitives/RecoveryTransport.h"
#include "../include/DeviceState.h"
#include <iostream>
#include <memory>
#include <sstream>

namespace PP {

namespace {

constexpr const char* kSupportDoc = "docs/SUPPORT.md";
constexpr const char* kIpwndfuUrl = "https://github.com/axi0mX/ipwndfu";

void logGen0UnsupportedSummary() {
    Logger::warn("Generation 0 jailbreak is NOT fully supported in purplepois0n.");
    Logger::warn("See " + std::string(kSupportDoc) + " for the capability matrix.");
}

void logDfuGap(const DFUDevice& device) {
    Logger::info("DFU device connected (bootrom-stage I/O available).");
    Logger::info("  Type: " + device.getDeviceType());
    try {
        Logger::info("  Serial: " + device.getSerialNumber());
    } catch (const std::exception&) {
        /* optional metadata */
    }
    Logger::warn("  greenpois0n era: limera1n / SHAtter (A4-class) — not implemented here.");
    Logger::info("  checkm8 (A5–A11): use -m/--checkm8 or jailbreak in DFU (requires gaster or ipwndfu).");
    Logger::info("  External tools: gaster (PURPLEPOIS0N_GASTER) or ipwndfu (PURPLEPOIS0N_IPWNDFU).");
    Logger::info("  Reference: " + std::string(kIpwndfuUrl));
}

void logRecoveryGap(uint64_t ecid, RecoveryDevice* device) {
    if (device != nullptr) {
        Logger::info("Recovery device connected (iBoot-stage I/O available).");
        try {
            Logger::info("  Type: " + device->getDeviceType());
        } catch (const std::exception&) {
            /* optional */
        }
        if (ecid != 0) {
            std::ostringstream oss;
            oss << "  ECID 0x" << std::hex << ecid << std::dec;
            Logger::info(oss.str());
        }
    } else {
        Logger::warn("Recovery mode detected but could not open RecoveryDevice.");
        if (ecid == 0) {
            Logger::warn("  ECID unknown — wire ECID from enumeration or irecv_get_ecid.");
        } else {
            std::ostringstream oss;
            oss << "  ECID 0x" << std::hex << ecid << std::dec;
            Logger::warn(oss.str());
        }
    }
    Logger::warn("iBoot exploit chain, IMG3/ramdisk load, and restore helpers — NOT implemented.");
    Logger::info("Historical greenpois0n used DFU bootrom entry first; Recovery is secondary.");
}

void logNormalGap(const MobileDevice& device) {
    Logger::info("Normal mode device connected.");
    Logger::info("  Name: " + device.getDeviceName());
    Logger::info("  Type: " + device.getDeviceType());
    Logger::info("  UDID: " + device.getUDID());
    std::string iosVersion = device.getValue("", "ProductVersion");
    if (!iosVersion.empty()) {
        Logger::info("  iOS: " + iosVersion);
    }
    Logger::warn("Userland / backup-mediated jailbreak (absinthe-style) — NOT implemented.");
    Logger::warn("  No backup restore, staging, or untether install in-tree.");
    Logger::info("Educational host tools:");
    Logger::info("  --analyze-backup PATH   parse Manifest.db / mbdb / plist offline");
    Logger::info("  --analyze-dyldcache PATH parse dyld shared cache offline");
    Logger::info("  AFCService (C++ API)    uploadFile / downloadFile when device is trusted");
    Logger::info("See " + std::string(kSupportDoc) + " and book/deep/normal-mode-afc-backup.md");
}

primitives::ExecutionContext makeContext(DeviceState state,
                                         const std::string& udid,
                                         uint64_t ecid,
                                         bool allowMutation) {
    primitives::ExecutionContext ctx;
    ctx.deviceState = state;
    ctx.udid = udid;
    ctx.ecid = ecid;
    ctx.allowMutation = allowMutation;
    return ctx;
}

void maybeWriteReport(primitives::ChainRunner& runner, const std::string& reportPath) {
    if (reportPath.empty()) {
        return;
    }
    if (runner.writeReportToFile(reportPath)) {
        Logger::info("Wrote chain report: " + reportPath);
    } else {
        Logger::warn("Failed to write chain report: " + reportPath);
    }
}

} /* anonymous namespace */

bool runGen0Jailbreak(DeviceManager& manager,
                      const std::string& targetUDID,
                      const Gen0Options& options) {
    Logger::info("Generation 0 workflow (greenpois0n / absinthe scaffold)");
    logGen0UnsupportedSummary();

    DeviceState state = manager.detectDeviceState(targetUDID);
    if (state == DeviceState::Unknown) {
        Logger::error("No device detected. Connect a device in Normal, Recovery, or DFU mode.");
        return false;
    }

    const char* stateLabel = "Unknown";
    switch (state) {
        case DeviceState::Normal:
            stateLabel = "Normal";
            break;
        case DeviceState::Recovery:
            stateLabel = "Recovery";
            break;
        case DeviceState::DFU:
            stateLabel = "DFU";
            break;
        default:
            break;
    }
    Logger::info(std::string("Detected mode: ") + stateLabel);

    try {
        primitives::ChainRunner runner;
        primitives::ExecutionContext ctx = makeContext(state, targetUDID, 0, false);

        switch (state) {
            case DeviceState::DFU: {
                auto device = manager.getDFUDevice();
                if (!device) {
                    Logger::error("Failed to open DFU device.");
                    return false;
                }
                logDfuGap(*device);
                primitives::DfuTransport transport(*device);
                ctx.transport = &transport;
                try {
                    ctx.cpid = Checkm8::parseCpidFromSerial(device->getSerialNumber());
                } catch (const std::exception&) {
                    /* optional */
                }
                runner.runProbeChain(ctx);
                break;
            }
            case DeviceState::Recovery: {
                const uint64_t ecid = manager.getRecoveryEcid();
                ctx.ecid = ecid;
                std::unique_ptr<RecoveryDevice> device = manager.getRecoveryDevice(ecid);
                logRecoveryGap(ecid, device.get());
                if (device) {
                    primitives::RecoveryTransport transport(*device);
                    ctx.transport = &transport;
                }
                runner.runProbeChain(ctx);
                break;
            }
            case DeviceState::Normal: {
                auto device = manager.getMobileDevice(targetUDID);
                if (!device) {
                    Logger::error("Failed to connect in normal mode (trust/unlock required).");
                    return false;
                }
                logNormalGap(*device);
                ctx.udid = device->getUDID();
                runner.runProbeChain(ctx);
                if (!options.backupPath.empty()) {
                    Logger::info("Running backup analysis from --gen0 hook.");
                    analyzeBackup(options.backupPath);
                }
                break;
            }
            default:
                Logger::error("Unknown device state.");
                return false;
        }

        maybeWriteReport(runner, options.reportPath);
    } catch (const std::exception& e) {
        Logger::error(std::string("Exception: ") + e.what());
        return false;
    }

    Logger::info("Gen 0 scaffold finished (no exploit or untether was applied).");
    return true;
}

bool analyzeBackup(const std::string& backupPath) {
    Logger::info("Analyzing backup (educational parse only; no restore): " + backupPath);

    try {
        MobileBackup backup(backupPath);
        if (!backup.isValid()) {
            Logger::error("Invalid or unreadable backup at: " + backupPath);
            return false;
        }

        std::cout << "\n=== MobileBackup analysis ===" << std::endl;
        std::cout << "Manifest:    " << backup.getManifestTypeName() << std::endl;
        const BackupProtocolInfo protocol = backup.getProtocolInfo();
        std::cout << "Index:       " << backupIndexProtocolName(protocol.index) << std::endl;
        std::cout << "Storage:     " << backupStorageProtocolName(protocol.storage);
        if (!protocol.statusVersion.empty()) {
            std::cout << " (Status.plist Version " << protocol.statusVersion << ")";
        }
        std::cout << std::endl;
        std::cout << "UDID:        " << backup.getUDID() << std::endl;
        std::cout << "Device:      " << backup.getDeviceName() << std::endl;
        std::cout << "iOS:         " << backup.getIOSVersion() << std::endl;
        std::cout << "Backup date: " << backup.getBackupDate() << std::endl;
        std::cout << "Encrypted:   " << (backup.isEncrypted() ? "yes" : "no") << std::endl;

        std::vector<std::string> domains = backup.getDomains();
        std::cout << "\nDomains (" << domains.size() << "):" << std::endl;
        const size_t kMaxDomainsListed = 32;
        for (size_t i = 0; i < domains.size() && i < kMaxDomainsListed; ++i) {
            auto files = backup.getFilesByDomain(domains[i]);
            std::cout << "  " << domains[i] << " (" << files.size() << " files)" << std::endl;
        }
        if (domains.size() > kMaxDomainsListed) {
            std::cout << "  ... and " << (domains.size() - kMaxDomainsListed) << " more domains"
                      << std::endl;
        }

        auto allFiles = backup.getAllFiles();
        const MobileBackup::Stats stats = backup.getStats();
        std::cout << "\nTotal manifest entries: " << allFiles.size() << std::endl;
        std::cout << "Entries with metadata:  " << stats.entriesWithMetadata << std::endl;
        std::cout << "Encrypted entries:      " << stats.encryptedEntries << std::endl;
        std::cout << "Directory placeholders: " << stats.directoryEntries << std::endl;

        if (backup.isEncrypted()) {
            std::cout << "\nEncrypted backup: file hashes are listed but content decrypt"
                         " is NOT implemented."
                      << std::endl;
        }
        std::cout << "\nAbsinthe-era note: historical tools used backup *restore* to stage"
                     " payloads."
                  << std::endl;
        std::cout << "purplepois0n only parses backups — see docs/SUPPORT.md." << std::endl;

        Logger::info("Backup analysis complete.");
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("Backup analysis failed: ") + e.what());
        return false;
    }
}

bool analyzeBinary(const std::string& binaryPath,
                   MachOArchPreference archPreference,
                   const std::string& payloadJsonPath) {
    Logger::info("Analyzing Mach-O (offline research): " + binaryPath);

    try {
        const std::unique_ptr<MachOBinary> binary =
            MachOBinary::open(binaryPath, archPreference);
        if (!binary || !binary->isValid()) {
            Logger::error("Invalid or unreadable Mach-O: " + binaryPath);
            return false;
        }

        std::cout << "\n=== Mach-O analysis ===" << std::endl;
        std::cout << "Path:          " << binaryPath << std::endl;
        std::cout << "Backend:       " << binary->backendName() << std::endl;
        std::cout << "Architecture:  " << binary->architectureName();
        if (archPreference != MachOArchPreference::Default) {
            std::cout << " (requested " << machOArchPreferenceName(archPreference) << ")";
        }
        std::cout << std::endl;

        if (binary->backend() == MachOBinary::Backend::Internal) {
            std::cout << "Bitness:       " << (binary->is64Bit() ? "64-bit" : "32-bit") << std::endl;
            std::cout << "Load commands: " << binary->loadCommandCount() << std::endl;

            const auto segments = binary->segments();
            std::cout << "Segments (" << segments.size() << "):" << std::endl;
            for (size_t i = 0; i < segments.size() && i < 16; ++i) {
                std::cout << "  " << segments[i].segname
                          << " vm=0x" << std::hex << segments[i].vmaddr << std::dec
                          << " file=" << segments[i].filesize << std::endl;
            }
            if (segments.size() > 16) {
                std::cout << "  ... and " << (segments.size() - 16) << " more" << std::endl;
            }

            const MachOSymtabInfo symtab = binary->symtabInfo();
            if (symtab.present) {
                std::cout << "Symtab:        " << symtab.nsyms << " symbols" << std::endl;
            }

            const MachODyldInfo dyld = binary->dyldInfo();
            if (dyld.present) {
                std::cout << "Dyld info:     rebase=" << dyld.rebaseSize
                          << " bind=" << dyld.bindSize
                          << " export=" << dyld.exportSize << std::endl;
            }

            if (binary->entryPoint() != 0) {
                std::cout << "Entry (LC_MAIN): 0x" << std::hex << binary->entryPoint()
                          << std::dec << std::endl;
            }

            const auto symbols = binary->symbols();
            std::cout << "Named symbols: " << symbols.size() << std::endl;
            const size_t kMaxSymbols = 8;
            for (size_t i = 0; i < symbols.size() && i < kMaxSymbols; ++i) {
                std::cout << "  " << symbols[i].name << std::endl;
            }
            if (symbols.size() > kMaxSymbols) {
                std::cout << "  ... and " << (symbols.size() - kMaxSymbols) << " more" << std::endl;
            }
        } else {
            std::cout << "Detail:        see JSON payload (ipswd / ipsw macho info)"
                      << std::endl;
        }

        if (!payloadJsonPath.empty()) {
            if (binary->writePayloadToFile(payloadJsonPath)) {
                Logger::info("Wrote analysis payload: " + payloadJsonPath);
            } else {
                Logger::warn("Failed to write analysis payload: " + payloadJsonPath);
            }
        }

        Logger::info("Mach-O analysis complete.");
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("Mach-O analysis failed: ") + e.what());
        return false;
    }
}

bool analyzeDyldCache(const std::string& cachePath, const std::string& payloadJsonPath) {
    Logger::info("Analyzing dyld shared cache (offline research): " + cachePath);

    try {
        const std::unique_ptr<DyldSharedCache> cache = DyldSharedCache::open(cachePath);
        if (!cache || !cache->isValid()) {
            Logger::error("Invalid or unreadable dyld cache: " + cachePath);
            return false;
        }

        std::cout << "\n=== dyld shared cache analysis ===" << std::endl;
        std::cout << "Path:          " << cachePath << std::endl;
        std::cout << "Backend:       " << cache->backendName() << std::endl;
        std::cout << "Magic:         " << cache->magicString() << std::endl;
        std::cout << "Architecture:  " << cache->architecture() << std::endl;
        std::cout << "UUID:          " << cache->uuid() << std::endl;
        std::cout << "Supported:     " << (cache->isSupportedVariant() ? "yes" : "partial")
                  << std::endl;
        std::cout << "Arm32 cache:   " << (cache->isArm32Cache() ? "yes" : "no") << std::endl;
        std::cout << "Arm64 cache:   " << (cache->isArm64Cache() ? "yes" : "no") << std::endl;
        std::cout << "Base address:  0x" << std::hex << cache->baseAddress() << std::dec
                  << std::endl;

        const auto mappings = cache->mappings();
        std::cout << "Mappings (" << mappings.size() << "):" << std::endl;
        const size_t kMaxMappings = 8;
        for (size_t i = 0; i < mappings.size() && i < kMaxMappings; ++i) {
            std::cout << "  vm=0x" << std::hex << mappings[i].address << std::dec
                      << " size=" << mappings[i].size
                      << " fileOff=" << mappings[i].fileOffset << std::endl;
        }
        if (mappings.size() > kMaxMappings) {
            std::cout << "  ... and " << (mappings.size() - kMaxMappings) << " more" << std::endl;
        }

        const auto images = cache->imageInfos();
        std::cout << "Images:        " << images.size() << std::endl;
        const size_t kMaxImages = 12;
        for (size_t i = 0; i < images.size() && i < kMaxImages; ++i) {
            std::cout << "  " << images[i].path << std::endl;
        }
        if (images.size() > kMaxImages) {
            std::cout << "  ... and " << (images.size() - kMaxImages) << " more" << std::endl;
        }

        if (cache->backend() == DyldSharedCache::Backend::Ipswd ||
            cache->backend() == DyldSharedCache::Backend::Ipsw) {
            std::cout << "Catalog:       ipswd/ipsw JSON payload (+ internal parser for layout)"
                      << std::endl;
        } else if (!cache->isSupportedVariant()) {
            std::cout << "\nNote: dyld_v2+ caches may parse headers only; image catalog can be"
                         " incomplete."
                      << std::endl;
        }

        if (!payloadJsonPath.empty()) {
            if (cache->writePayloadToFile(payloadJsonPath)) {
                Logger::info("Wrote analysis payload: " + payloadJsonPath);
            } else {
                Logger::warn("Failed to write analysis payload: " + payloadJsonPath);
            }
        }

        Logger::info("dyld cache analysis complete.");
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("dyld cache analysis failed: ") + e.what());
        return false;
    }
}

} /* namespace PP */
