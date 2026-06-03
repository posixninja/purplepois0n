/*
 * purplepois0n.cpp
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>
#include "DeviceManager.h"
#include "Gen0Workflow.h"
#include "Checkm8.h"
#include "Logger.h"
#include "MachOParser.h"
#include "DFUDevice.h"
#include "primitives/ChainRunner.h"
#include "primitives/DfuTransport.h"
#include "primitives/PrimitiveTypes.h"
#include "../include/DeviceState.h"

using namespace PP;
using namespace PP::primitives;

static void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "purplepois0n is a research framework. Generation 0 (greenpois0n / absinthe)"
              << std::endl;
    std::cout << "is not a complete jailbreak — see docs/SUPPORT.md." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help                 Show this help message" << std::endl;
    std::cout << "  -v, --verbose              Enable verbose logging" << std::endl;
    std::cout << "  -l, --list                 List all connected devices" << std::endl;
    std::cout << "  -d, --device UDID          Target specific device by UDID (normal mode)"
              << std::endl;
    std::cout << "  -j, --jailbreak            Run jailbreak scaffold (DFU: probe; other: Gen0)"
              << std::endl;
    std::cout << "  -m, --checkm8              Run checkm8 bootrom exploit (DFU only)" << std::endl;
    std::cout << "  --gen0                     Generation 0 scaffold (DFU/Recovery/Normal gaps)"
              << std::endl;
    std::cout << "  --analyze-backup PATH      Parse iTunes backup offline (no restore)"
              << std::endl;
    std::cout << "  --analyze-binary PATH      Parse Mach-O binary offline (segments/symbols)"
              << std::endl;
    std::cout << "  --analyze-dyldcache PATH   Parse dyld shared cache offline (image catalog)"
              << std::endl;
    std::cout << "  --analyze-json FILE        Write ipsw/internal JSON payload (with analyze flags)"
              << std::endl;
    std::cout << "  --arch arm32|arm64         Slice preference for --analyze-binary fat Mach-O"
              << std::endl;
    std::cout << "  --report FILE              Write ChainRunner JSON report to FILE"
              << std::endl;
    std::cout << std::endl;
    std::cout << "Framework APIs (not all exposed on CLI):" << std::endl;
    std::cout << "  AFCService                 uploadFile / downloadFile (trusted normal mode)"
              << std::endl;
    std::cout << "  MachOParser                segments, symtab, dyld info (offline)"
              << std::endl;
    std::cout << "  MobileBackup               manifest domains, extractFile (offline)"
              << std::endl;
    std::cout << "  primitives::ChainRunner    staged probe/execute primitive chain"
              << std::endl;
}

static void listDevices(DeviceManager& manager) {
    Logger::info("Enumerating connected devices...");
    
    std::vector<DeviceInfo> devices = manager.enumerateDevices();
    
    if (devices.empty()) {
        Logger::warn("No devices found");
        return;
    }
    
    std::cout << "\nFound " << devices.size() << " device(s):" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (size_t i = 0; i < devices.size(); i++) {
        const auto& device = devices[i];
        std::cout << "Device " << (i + 1) << ":" << std::endl;
        std::cout << "  UDID: " << (device.udid.empty() ? "N/A" : device.udid) << std::endl;
        std::cout << "  ECID: 0x" << std::hex << std::uppercase << device.ecid << std::dec << std::endl;
        
        std::string stateStr;
        switch (device.state) {
            case DeviceState::Normal:
                stateStr = "Normal";
                break;
            case DeviceState::Recovery:
                stateStr = "Recovery";
                break;
            case DeviceState::DFU:
                stateStr = "DFU";
                break;
            default:
                stateStr = "Unknown";
                break;
        }
        std::cout << "  State: " << stateStr << std::endl;
        std::cout << "  Type: " << (device.deviceType.empty() ? "N/A" : device.deviceType) << std::endl;
        std::cout << "  Firmware: " << (device.firmwareVersion.empty() ? "N/A" : device.firmwareVersion) << std::endl;
        if (device.cpid != 0) {
            std::cout << "  CPID: 0x" << std::hex << std::uppercase << device.cpid << std::dec << std::endl;
        }
        std::cout << std::endl;
    }
}

static bool runDfuChain(DeviceManager& manager, bool allowMutation, const std::string& reportPath) {
    auto device = manager.getDFUDevice();
    if (!device) {
        Logger::error("Failed to open DFU device.");
        return false;
    }

    DfuTransport transport(*device);
    ExecutionContext ctx;
    ctx.deviceState = DeviceState::DFU;
    ctx.transport = &transport;
    ctx.allowMutation = allowMutation;
    try {
        ctx.cpid = Checkm8::parseCpidFromSerial(device->getSerialNumber());
    } catch (const std::exception&) {
        /* optional */
    }

    ChainRunner runner;
    runner.runProbeChain(ctx);
    if (!reportPath.empty()) {
        if (runner.writeReportToFile(reportPath)) {
            Logger::info("Wrote chain report: " + reportPath);
        } else {
            Logger::warn("Failed to write chain report: " + reportPath);
        }
    }

    if (!allowMutation) {
        Logger::info("DFU probe complete — use -m/--checkm8 to run bootrom exploit.");
        return true;
    }

    device.reset();

    if (exploitPluginsEnabled()) {
        ChainRunner executeRunner;
        ctx.transport = nullptr;
        ctx.allowMutation = true;
        executeRunner.runExecuteChain(ctx);
    } else {
        Logger::warn("Mutating bootrom primitives require PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS at compile time.");
        Logger::info("Delegating to Checkm8::runCheckm8 (external gaster/ipwndfu).");
    }

    return Checkm8::runCheckm8(manager);
}

static bool performJailbreak(DeviceManager& manager,
                             const std::string& targetUDID,
                             const std::string& reportPath) {
    Logger::info("Starting jailbreak process...");
    const DeviceState state = manager.detectDeviceState(targetUDID);
    if (state == DeviceState::DFU) {
        Logger::info("DFU mode — running primitive probe chain (no auto-pwn)");
        return runDfuChain(manager, false, reportPath);
    }
    Logger::info("Non-DFU mode — Gen 0 scaffold");
    Gen0Options options;
    options.reportPath = reportPath;
    return runGen0Jailbreak(manager, targetUDID, options);
}

int main(int argc, char* argv[]) {
    bool listDevicesFlag = false;
    bool jailbreakFlag = true;
    bool gen0Flag = false;
    bool checkm8Flag = false;
    bool analyzeBackupFlag = false;
    bool analyzeBinaryFlag = false;
    bool analyzeDyldCacheFlag = false;
    bool verbose = false;
    std::string targetUDID;
    std::string backupPath;
    std::string binaryPath;
    std::string dyldCachePath;
    std::string analyzeJsonPath;
    std::string reportPath;
    std::string archPreferenceStr;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
            Logger::setLogLevel(LogLevel::DEBUG);
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
            listDevicesFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--device") == 0) {
            if (i + 1 < argc) {
                targetUDID = argv[++i];
            } else {
                Logger::error("--device requires a UDID argument");
                return 1;
            }
        } else if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--jailbreak") == 0) {
            jailbreakFlag = true;
        } else if (strcmp(argv[i], "--gen0") == 0) {
            gen0Flag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--checkm8") == 0) {
            checkm8Flag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--analyze-backup") == 0) {
            if (i + 1 < argc) {
                backupPath = argv[++i];
                if (!gen0Flag) {
                    analyzeBackupFlag = true;
                    jailbreakFlag = false;
                }
            } else {
                Logger::error("--analyze-backup requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--analyze-binary") == 0) {
            if (i + 1 < argc) {
                binaryPath = argv[++i];
                analyzeBinaryFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--analyze-binary requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--analyze-dyldcache") == 0) {
            if (i + 1 < argc) {
                dyldCachePath = argv[++i];
                analyzeDyldCacheFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--analyze-dyldcache requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--analyze-json") == 0) {
            if (i + 1 < argc) {
                analyzeJsonPath = argv[++i];
            } else {
                Logger::error("--analyze-json requires a FILE argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--arch") == 0) {
            if (i + 1 < argc) {
                archPreferenceStr = argv[++i];
            } else {
                Logger::error("--arch requires arm32 or arm64");
                return 1;
            }
        } else if (strcmp(argv[i], "--report") == 0) {
            if (i + 1 < argc) {
                reportPath = argv[++i];
            } else {
                Logger::error("--report requires a FILE argument");
                return 1;
            }
        } else {
            Logger::error("Unknown option: " + std::string(argv[i]));
            printUsage(argv[0]);
            return 1;
        }
    }
    
    (void)verbose;

    Logger::info("purplepois0n - iOS Jailbreak Tool");
    Logger::info("=================================");
    
    DeviceManager manager;
    
    if (listDevicesFlag) {
        listDevices(manager);
        return 0;
    }

    if (analyzeBackupFlag) {
        return analyzeBackup(backupPath) ? 0 : 1;
    }

    if (analyzeBinaryFlag) {
        return analyzeBinary(binaryPath, machOArchPreferenceFromString(archPreferenceStr),
                            analyzeJsonPath)
                   ? 0
                   : 1;
    }

    if (analyzeDyldCacheFlag) {
        return analyzeDyldCache(dyldCachePath, analyzeJsonPath) ? 0 : 1;
    }

    if (gen0Flag) {
        Gen0Options options;
        options.reportPath = reportPath;
        options.backupPath = backupPath;
        return runGen0Jailbreak(manager, targetUDID, options) ? 0 : 1;
    }

    if (checkm8Flag) {
        const DeviceState state = manager.detectDeviceState(targetUDID);
        if (state != DeviceState::DFU) {
            Logger::error("checkm8 requires a device in DFU mode.");
            return 1;
        }
        return runDfuChain(manager, true, reportPath) ? 0 : 1;
    }
    
    if (jailbreakFlag) {
        if (!performJailbreak(manager, targetUDID, reportPath)) {
            return 1;
        }
    }
    
    return 0;
}
