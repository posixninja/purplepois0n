/*
 * Gen0Workflow.cpp
 */

#include "Gen0Workflow.h"
#include "Gen0Context.h"
#include "DeviceManager.h"
#include "Logger.h"
#include "MobileBackup.h"
#include "BackupProtocol.h"
#include "MachOBinary.h"
#include "DyldSharedCache.h"
#include "CrashSlideHelper.h"
#include "DFUDevice.h"
#include "RecoveryDevice.h"
#include "MobileDevice.h"
#include "Checkm8.h"
#include "primitives/ChainRunner.h"
#include "primitives/DfuTransport.h"
#include "primitives/RecoveryTransport.h"
#include "primitives/Gen6Types.h"
#include "primitives/TssTypes.h"
#include "primitives/TssDelegate.h"
#include "primitives/CodesignDelegate.h"
#include "primitives/CodesignTypes.h"
#include "primitives/TrustCacheDelegate.h"
#include "primitives/SideloadPrimitive.h"
#include "primitives/PongoDelegate.h"
#include "IpaSignHelper.h"
#include "RamdiskPackager.h"
#include "RamdiskWorkDir.h"
#include "RamdiskInspector.h"
#include "RamdiskClient.h"
#include "../include/DeviceState.h"
#include <fstream>
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
    Logger::info("  old-BR 3GS / iPod 2G: 24kpwn (0x24000) untether — gen0-24kpwn stub / PURPLEPOIS0N_24KPWN.");
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
    Logger::warn("iBoot exploit chain and restore FSM — NOT implemented.");
    Logger::info("Recovery ramdisk builder + multi-stage chain — Partial (see recovery-ramdisk.md).");
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
    Logger::warn("  Gen 6 rootless (Dopamine / TrollStore) — on-device chain; host runs Gen6 primitives in --gen0.");
    Logger::warn("  No backup restore, staging, or untether install in-tree.");
    Logger::info("Educational host tools:");
    Logger::info("  --analyze-backup PATH   parse Manifest.db / mbdb / plist offline");
    Logger::info("  --analyze-dyldcache PATH parse dyld shared cache offline");
    Logger::info("  AFCService (C++ API)    uploadFile / downloadFile when device is trusted");
    Logger::info("See " + std::string(kSupportDoc) + " and book/deep/normal-mode-afc-backup.md");
}

bool inferArm64e(const std::string& productType, const std::string& iosVersion) {
    if (productType.find("iPhone11,") == 0 || productType.find("iPhone12,") == 0 ||
        productType.find("iPhone13,") == 0 || productType.find("iPhone14,") == 0 ||
        productType.find("iPhone15,") == 0 || productType.find("iPhone16,") == 0 ||
        productType.find("iPhone17,") == 0) {
        return true;
    }
    if (productType.find("iPad8,") == 0 || productType.find("iPad11,") == 0 ||
        productType.find("iPad12,") == 0 || productType.find("iPad13,") == 0) {
        return true;
    }
    (void)iosVersion;
    return false;
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
        primitives::ExecutionContext ctx =
            buildExecutionContext(state, options, targetUDID, 0, false);
        bool workflowOk = true;

        if (ctx.recoveryChain.empty() && ctx.recoveryChainRun && !ctx.ipswPath.empty()) {
            const std::string workDir = resolveRamdiskWorkDir(ctx.ramdiskWorkDir);
            populateDefaultRecoveryChain(ctx.ipswPath, workDir, &ctx.recoveryChain);
        }

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
                ctx.jailbreakGeneration =
                    primitives::detectJailbreakGeneration(ctx);
                runner.runProbeChain(ctx);
                if (options.pongo.execute &&
                    (options.pongo.bootRun || options.pongo.probeRun)) {
                    ctx.allowMutation = true;
                    if (primitives::exploitPluginsEnabled() &&
                        (options.pongo.spawnCheckra1n || options.pongo.bootRun)) {
                        const primitives::PrimitiveResult spawn =
                            primitives::PongoDelegate::spawnCheckra1nShell(true);
                        if (spawn != primitives::PrimitiveResult::Success) {
                            workflowOk = false;
                        }
                    }
                    if (workflowOk &&
                        !runner.runPongoMiniChain(ctx, true)) {
                        workflowOk = false;
                    }
                }
                break;
            }
            case DeviceState::Recovery: {
                const uint64_t ecid = manager.getRecoveryEcid();
                ctx.ecid = ecid;
                std::unique_ptr<RecoveryDevice> device = manager.getRecoveryDevice(ecid);
                logRecoveryGap(ecid, device.get());
                if (device) {
                    ctx.cpid = device->getCpid();
                    ctx.boardId = device->getBoardId();
                    if (ctx.productType.empty()) {
                        ctx.productType = device->getDeviceType();
                    }
                    if (ctx.iosVersion.empty()) {
                        ctx.iosVersion = device->getFirmwareVersion();
                    }
                    primitives::RecoveryTransport transport(*device);
                    ctx.transport = &transport;
                }
                ctx.backupPath = options.backupPath;
                ctx.jailbreakGeneration =
                    primitives::detectJailbreakGeneration(ctx);
                runner.runProbeChain(ctx);
                if (options.recovery.execute) {
                    ctx.allowMutation = true;
                    if (!runner.runExecuteChain(ctx)) {
                        workflowOk = false;
                    }
                }
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
                ctx.iosVersion = device->getValue("", "ProductVersion");
                ctx.productType = device->getValue("", "ProductType");
                ctx.arm64e = inferArm64e(ctx.productType, ctx.iosVersion);
                ctx.backupPath = options.backupPath;
                ctx.jailbreakGeneration =
                    primitives::detectJailbreakGeneration(ctx);
                runner.runProbeChain(ctx);
                if (options.postJbPipeline && primitives::exploitPluginsEnabled()) {
                    if (!runPostJbPipeline(manager, targetUDID, options)) {
                        workflowOk = false;
                    }
                }
                break;
            }
            default:
                Logger::error("Unknown device state.");
                return false;
        }

        maybeWriteReport(runner, options.reportPath);
        if (!workflowOk) {
            return false;
        }
    } catch (const std::exception& e) {
        Logger::error(std::string("Exception: ") + e.what());
        return false;
    }

    Logger::info("Gen 0 scaffold finished (no exploit or untether was applied).");
    return true;
}

bool runTssCheck(DeviceManager& manager,
                 const std::string& targetUDID,
                 const std::string& productType,
                 const std::string& iosVersion,
                 uint64_t ecid,
                 const std::string& ipswPath,
                 const std::string& apticketPath) {
    Logger::info("TSS / futurerestore signing probe (no restore)");

    primitives::ExecutionContext ctx;
    ctx.deviceState = DeviceState::Unknown;
    ctx.productType = productType;
    ctx.iosVersion = iosVersion;
    ctx.ecid = ecid;
    ctx.ipswPath = ipswPath;
    ctx.apticketPath = apticketPath;
    ctx.futureRestore = primitives::futureRestoreOptionsFromEnv();
    if (!apticketPath.empty()) {
        ctx.futureRestore.apticketPath = apticketPath;
    }

    if (!targetUDID.empty()) {
        const DeviceState state = manager.detectDeviceState(targetUDID);
        ctx.deviceState = state;
        if (state == DeviceState::Normal) {
            try {
                auto device = manager.getMobileDevice(targetUDID);
                if (device) {
                    ctx.udid = device->getUDID();
                    if (ctx.iosVersion.empty()) {
                        ctx.iosVersion = device->getValue("", "ProductVersion");
                    }
                    if (ctx.productType.empty()) {
                        ctx.productType = device->getValue("", "ProductType");
                    }
                }
            } catch (const std::exception& e) {
                Logger::error(std::string("Normal connect failed: ") + e.what());
                return false;
            }
        } else if (state == DeviceState::Recovery && ecid != 0) {
            ctx.ecid = ecid;
        }
    }

    const bool savedTicketPlan =
        !ctx.ipswPath.empty() &&
        primitives::TssDelegate::resolveSigningMode(ctx) == primitives::TssSigningMode::SavedApTicket;

    if (ctx.productType.empty() || ctx.iosVersion.empty()) {
        if (!savedTicketPlan) {
            Logger::error("Need -d UDID (Normal) or pass ProductType + iOS version for signing check");
            primitives::TssDelegate::logProcessOverview(ctx);
            return false;
        }
        primitives::TssDelegate::logProcessOverview(ctx);
    }

    primitives::PrimitiveResult probeResult = primitives::PrimitiveResult::Success;
    if (!ctx.productType.empty() && !ctx.iosVersion.empty()) {
        probeResult = primitives::TssDelegate::probe(ctx);
        if (probeResult != primitives::PrimitiveResult::Success) {
            return false;
        }
    }

    if (savedTicketPlan) {
        if (!primitives::TssDelegate::isFuturerestoreConfigured()) {
            Logger::error("  [TSS] futurerestore required for saved-blob restore planning");
            return false;
        }
        primitives::TssDelegate::runFuturerestoreRestore(ctx, false);
    }

    return true;
}

bool fetchLiveShsh(DeviceManager& manager,
                   const std::string& targetUDID,
                   const std::string& ipswPath,
                   const std::string& outputPath) {
    Logger::info("Fetching live SHSH (libtatsu / ipsw) — no restore");

    primitives::ExecutionContext ctx;
    ctx.ipswPath = ipswPath;
    ctx.futureRestore = primitives::futureRestoreOptionsFromEnv();

    if (!targetUDID.empty()) {
        const DeviceState state = manager.detectDeviceState(targetUDID);
        ctx.deviceState = state;
        if (state == DeviceState::Normal) {
            try {
                auto device = manager.getMobileDevice(targetUDID);
                if (device) {
                    ctx.udid = device->getUDID();
                    ctx.iosVersion = device->getValue("", "ProductVersion");
                    ctx.productType = device->getValue("", "ProductType");
                }
            } catch (const std::exception& e) {
                Logger::error(std::string("Normal connect failed: ") + e.what());
                return false;
            }
        } else if (state == DeviceState::Recovery) {
            const uint64_t ecid = manager.getRecoveryEcid();
            ctx.ecid = ecid;
            try {
                std::unique_ptr<RecoveryDevice> device = manager.getRecoveryDevice(ecid);
                if (device) {
                    ctx.cpid = device->getCpid();
                    ctx.boardId = device->getBoardId();
                    ctx.productType = device->getDeviceType();
                    ctx.iosVersion = device->getFirmwareVersion();
                    primitives::RecoveryTransport transport(*device);
                    ctx.transport = &transport;
                    return primitives::TssDelegate::fetchLiveShsh(ctx, outputPath) ==
                           primitives::PrimitiveResult::Success;
                }
            } catch (const std::exception& e) {
                Logger::error(std::string("Recovery connect failed: ") + e.what());
                return false;
            }
        }
    }

    return primitives::TssDelegate::fetchLiveShsh(ctx, outputPath) ==
           primitives::PrimitiveResult::Success;
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

bool analyzeCrashLog(const std::string& crashPath) {
    Logger::info("Analyzing crash log (research only; no staging): " + crashPath);

    CrashSlideSummary summary;
    if (!parseCrashSlideFile(crashPath, &summary)) {
        Logger::error("No slide annotations found in: " + crashPath);
        return false;
    }

    printCrashSlideSummary(summary, std::cout);
    Logger::info("Crash slide analysis complete.");
    return true;
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

bool runHostCodesign(const std::string& inputPath,
                     const primitives::CodesignOptions& options,
                     bool allowMutation) {
    if (inputPath.empty()) {
        Logger::error("codesign requires an input path");
        return false;
    }
    primitives::CodesignOptions opts =
        primitives::CodesignDelegate::mergeOptions(options, primitives::codesignOptionsFromEnv());
    return primitives::CodesignDelegate::signPath(opts, inputPath, allowMutation) ==
           primitives::PrimitiveResult::Success;
}

bool runHostSignIpa(const std::string& ipaPath,
                    const std::string& outputIpaPath,
                    const primitives::CodesignOptions& options,
                    bool allowMutation) {
    if (ipaPath.empty()) {
        Logger::error("sign-ipa requires an IPA path");
        return false;
    }
    primitives::CodesignOptions opts =
        primitives::CodesignDelegate::mergeOptions(options, primitives::codesignOptionsFromEnv());
    return !signIpa(opts, ipaPath, outputIpaPath, allowMutation).empty();
}

bool runSideloadInstall(DeviceManager& manager,
                         const std::string& targetUDID,
                         const std::string& ipaPath,
                         bool allowMutation) {
    if (targetUDID.empty()) {
        Logger::error("install-ipa requires -d UDID");
        return false;
    }
    if (manager.detectDeviceState(targetUDID) != DeviceState::Normal) {
        Logger::error("install-ipa requires trusted Normal mode");
        return false;
    }
    try {
        auto device = manager.getMobileDevice(targetUDID);
        if (!device) {
            Logger::error("Failed to connect in normal mode");
            return false;
        }
        primitives::ExecutionContext ctx;
        ctx.deviceState = DeviceState::Normal;
        ctx.udid = device->getUDID();
        ctx.ipaInstallPath = ipaPath;
        ctx.allowMutation = allowMutation;
        primitives::SideloadPrimitive primitive;
        return primitive.execute(ctx) == primitives::PrimitiveResult::Success;
    } catch (const std::exception& e) {
        Logger::error(std::string("install-ipa failed: ") + e.what());
        return false;
    }
}

bool runTrustCacheAdd(const std::string& binaryPath, bool allowMutation) {
    primitives::ExecutionContext ctx;
    ctx.trustCachePath = binaryPath;
    ctx.allowMutation = allowMutation;
    return runTrustCacheAddWithContext(ctx, allowMutation);
}

bool runTrustCacheAddWithContext(const primitives::ExecutionContext& context, bool allowMutation) {
    primitives::ExecutionContext ctx = context;
    ctx.allowMutation = allowMutation;
    return primitives::TrustCacheDelegate::addToTrustCache(ctx, allowMutation) ==
           primitives::PrimitiveResult::Success;
}

bool runPostJbPipeline(DeviceManager& manager,
                       const std::string& targetUDID,
                       const Gen0Options& options) {
    Logger::info("Post-jailbreak pipeline: sign → install → trustcache");

    if (!primitives::exploitPluginsEnabled()) {
        Logger::error("Post-jb pipeline requires make plugins");
        return false;
    }

    std::string ipaToInstall = options.ipaInstallPath;
    if (ipaToInstall.empty()) {
        Logger::error("Post-jb pipeline requires --install-ipa PATH");
        return false;
    }

    if (!options.codesignInputPath.empty()) {
        const std::string signedOut =
            options.codesignOutputPath.empty() ? (ipaToInstall + ".signed") : options.codesignOutputPath;
        if (!runHostSignIpa(options.codesignInputPath, signedOut, options.codesign, true)) {
            return false;
        }
        ipaToInstall = signedOut;
    }

    if (!runSideloadInstall(manager, targetUDID, ipaToInstall, true)) {
        return false;
    }

    if (!options.trustCachePath.empty()) {
        RamdiskConnectOptions connect = options.ramdisk.connect;
        if (connect.udid.empty() && !targetUDID.empty()) {
            connect.udid = targetUDID;
        }
        primitives::ExecutionContext ctx;
        ctx.deviceState = DeviceState::Normal;
        ctx.trustCachePath = options.trustCachePath;
        ctx.ramdiskConnect = connect;
        ctx.allowMutation = true;
        if (!runTrustCacheAddWithContext(ctx, true)) {
            return false;
        }
    }

    Logger::info("Post-jb pipeline complete");
    return true;
}

bool runFuturerestoreRestore(const Gen0Options& options, bool allowMutation) {
    Logger::info("futurerestore restore (destructive — user-initiated only)");

    if (options.ipswPath.empty()) {
        Logger::error("futurerestore restore requires --ipsw PATH");
        return false;
    }
    if (options.apticketPath.empty() &&
        options.futureRestore.apticketPath.empty()) {
        Logger::error("futurerestore restore requires --apticket PATH");
        return false;
    }

    primitives::ExecutionContext ctx;
    ctx.ipswPath = options.ipswPath;
    ctx.apticketPath = options.apticketPath;
    ctx.futureRestore = options.futureRestore;
    if (ctx.futureRestore.apticketPath.empty() && !ctx.apticketPath.empty()) {
        ctx.futureRestore.apticketPath = ctx.apticketPath;
    }
    ctx.allowMutation = allowMutation;

    if (!primitives::TssDelegate::isFuturerestoreConfigured()) {
        Logger::error("futurerestore not found — set PURPLEPOIS0N_FUTURERESTORE");
        return false;
    }

    return primitives::TssDelegate::runFuturerestoreRestore(ctx, allowMutation) ==
           primitives::PrimitiveResult::Success;
}

bool runBuildRamdisk(const RamdiskOptions& options, const std::string& overlayDir,
                     const std::vector<RamdiskStageEntry>& stagedFiles,
                     const std::string& outputPath) {
    if (outputPath.empty()) {
        Logger::error("build-ramdisk requires --build-ramdisk PATH or --output");
        return false;
    }
    Logger::info("Building in-memory HFS+ ramdisk (no hdiutil mount)");
    if (!buildRamdiskDmg(options, overlayDir, stagedFiles, outputPath)) {
        return false;
    }
    if (!ramdiskLooksLikeHfsPlus(outputPath)) {
        Logger::warn("  [Ramdisk] H+ signature check failed at offset 1024");
        return false;
    }
    Logger::info("  [Ramdisk] HFS+ volume ready: " + outputPath);
    return true;
}

bool runRamdiskFromIpsw(const RamdiskOptions& options, const std::string& ipswPath,
                        const std::string& ident, const std::string& overlayDir,
                        const std::vector<RamdiskStageEntry>& stagedFiles,
                        const std::string& workDir, const std::string& outputPath) {
    if (ipswPath.empty()) {
        Logger::error("ramdisk-from-ipsw requires --ipsw");
        return false;
    }
    const std::string resolvedWork = resolveRamdiskWorkDir(workDir);
    RamdiskPackagerResult result;
    const std::string variant = ident.empty() ? std::string("Erase") : ident;
    if (!packRamdiskFromIpsw(options, ipswPath, variant, overlayDir, stagedFiles, resolvedWork,
                             &result)) {
        return false;
    }
    const std::string dest = outputPath.empty() ? result.im4pPath : outputPath;
    if (dest != result.im4pPath) {
        std::ifstream in(result.im4pPath.c_str(), std::ios::binary);
        std::ofstream out(dest.c_str(), std::ios::binary | std::ios::trunc);
        if (!in.is_open() || !out.is_open()) {
            Logger::error("  [Ramdisk] failed to copy IM4P to " + dest);
            return false;
        }
        out << in.rdbuf();
    }
    Logger::info("  [Ramdisk] packaged rdsk IM4P → " + dest);
    return true;
}

bool populateDefaultRecoveryChain(
    const std::string& ipswPath, const std::string& workDir,
    std::vector<primitives::ExecutionContext::RecoveryChainComponent>* chain) {
    if (chain == nullptr || ipswPath.empty()) {
        return false;
    }
    ensureDirectory(workDir);
    chain->clear();

    std::string ibssPath;
    if (locateIpswIm4pComponent(ipswPath, workDir, "iBSS.*im4p", &ibssPath)) {
        primitives::ExecutionContext::RecoveryChainComponent ibss;
        ibss.fourcc = "iBSS";
        ibss.ipswComponentPath = ibssPath;
        chain->push_back(ibss);
    }

    std::string ibecPath;
    if (locateIpswIm4pComponent(ipswPath, workDir + "/ibec", "iBEC.*im4p", &ibecPath)) {
        primitives::ExecutionContext::RecoveryChainComponent ibec;
        ibec.fourcc = "iBEC";
        ibec.ipswComponentPath = ibecPath;
        chain->push_back(ibec);
    }

    primitives::ExecutionContext::RecoveryChainComponent rdsk;
    rdsk.fourcc = "RestoreRamDisk";
    chain->push_back(rdsk);

    Logger::info("  [Recovery] default chain: " + std::to_string(chain->size()) + " stage(s)");
    return !chain->empty();
}

bool runRamdiskProbe(const RamdiskConnectOptions& options, std::string* message) {
    RamdiskClient client(options);
    return client.probe(message);
}

bool runRamdiskExec(const RamdiskConnectOptions& options, const std::string& command) {
    if (command.empty()) {
        Logger::error("ramdisk-exec requires a command");
        return false;
    }
    RamdiskClient client(options);
    std::string probeMsg;
    if (!client.probe(&probeMsg)) {
        Logger::error("  [Ramdisk] SSH unavailable: " + probeMsg);
        Logger::info("  [Ramdisk] note: stock IPSW RestoreRamDisk has no agent — use --ramdisk-overlay");
        Logger::info("  [Ramdisk] default transport is TCP (4444); use --ramdisk-transport ssh for sshd");
        return false;
    }
    const RamdiskCommandResult result = client.exec(command);
    if (!result.stdoutText.empty()) {
        std::cout << result.stdoutText;
    }
    if (result.exitCode != 0) {
        Logger::error("  [Ramdisk] exec failed: " + result.stderrText);
        return false;
    }
    return true;
}

bool runRamdiskPush(const RamdiskConnectOptions& options, const std::string& localPath,
                    const std::string& remotePath) {
    if (localPath.empty() || remotePath.empty()) {
        Logger::error("ramdisk-push requires LOCAL and REMOTE paths");
        return false;
    }
    RamdiskClient client(options);
    std::string probeMsg;
    if (!client.probe(&probeMsg)) {
        Logger::error("  [Ramdisk] SSH unavailable: " + probeMsg);
        Logger::info("  [Ramdisk] note: stock IPSW RestoreRamDisk has no agent — use --ramdisk-overlay");
        Logger::info("  [Ramdisk] default transport is TCP (4444); use --ramdisk-transport ssh for sshd");
        return false;
    }
    return client.uploadFile(localPath, remotePath);
}

bool runRamdiskPull(const RamdiskConnectOptions& options, const std::string& remotePath,
                    const std::string& localPath) {
    if (localPath.empty() || remotePath.empty()) {
        Logger::error("ramdisk-pull requires REMOTE and LOCAL paths");
        return false;
    }
    RamdiskClient client(options);
    std::string probeMsg;
    if (!client.probe(&probeMsg)) {
        Logger::error("  [Ramdisk] SSH unavailable: " + probeMsg);
        Logger::info("  [Ramdisk] note: stock IPSW RestoreRamDisk has no agent — use --ramdisk-overlay");
        Logger::info("  [Ramdisk] default transport is TCP (4444); use --ramdisk-transport ssh for sshd");
        return false;
    }
    return client.downloadFile(remotePath, localPath);
}

bool runRamdiskList(const RamdiskConnectOptions& options, const std::string& remotePath) {
    RamdiskClient client(options);
    std::string probeMsg;
    if (!client.probe(&probeMsg)) {
        Logger::error("  [Ramdisk] SSH unavailable: " + probeMsg);
        Logger::info("  [Ramdisk] note: stock IPSW RestoreRamDisk has no agent — use --ramdisk-overlay");
        Logger::info("  [Ramdisk] default transport is TCP (4444); use --ramdisk-transport ssh for sshd");
        return false;
    }
    const RamdiskCommandResult result = client.listDirectory(remotePath);
    if (result.exitCode != 0) {
        Logger::error("  [Ramdisk] ls failed: " + result.stderrText);
        return false;
    }
    std::cout << result.stdoutText;
    return true;
}

} /* namespace PP */
