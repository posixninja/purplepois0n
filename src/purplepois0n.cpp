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
#include "Gen0CliOptions.h"
#include "Checkm8.h"
#include "Logger.h"
#include "MachOParser.h"
#include "DFUDevice.h"
#include "AFCService.h"
#include "MobileDevice.h"
#include "primitives/ChainRunner.h"
#include "primitives/DfuTransport.h"
#include "primitives/PrimitiveTypes.h"
#include "primitives/Gen6Types.h"
#include "primitives/TssTypes.h"
#include "primitives/CodesignTypes.h"
#include "RamdiskTypes.h"
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
    std::cout << "  --analyze-crash PATH       Parse crash log for ASLR slide (research only)"
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
    std::cout << "  --afc-list REMOTE          List AFC directory (requires -d UDID, Normal mode)"
              << std::endl;
    std::cout << "  --afc-push LOCAL REMOTE    Upload file via AFC (requires -d UDID)"
              << std::endl;
    std::cout << "  --afc-pull REMOTE LOCAL    Download file via AFC (requires -d UDID)"
              << std::endl;
    std::cout << "  --tss-check                Probe TSS tools; check signing (needs -d or metadata)"
              << std::endl;
    std::cout << "  --ipsw PATH                Target IPSW (Gen0 / futurerestore probes)"
              << std::endl;
    std::cout << "  --apticket PATH            Saved SHSH/APTicket for futurerestore path"
              << std::endl;
    std::cout << "  --latest-sep               futurerestore: use latest signed SEP ticket"
              << std::endl;
    std::cout << "  --latest-baseband          futurerestore: use latest signed baseband ticket"
              << std::endl;
    std::cout << "  --no-baseband              futurerestore: skip baseband (Wi‑Fi-only devices)"
              << std::endl;
    std::cout << "  --sep-ipsw PATH            IPSW for latest SEP BuildManifest (libtatsu / futurerestore)"
              << std::endl;
    std::cout << "  --bb-ipsw PATH             IPSW for latest baseband BuildManifest"
              << std::endl;
    std::cout << "  --im4m-manifest PATH       IM4M for IMG4 personalize (or extract from --apticket)"
              << std::endl;
    std::cout << "  --ipsw-component PATH      Unsigned iBSS/iBEC .im4p to personalize then upload"
              << std::endl;
    std::cout << "  --recovery-upload PATH       Pre-signed component for Recovery irecv_send_file"
              << std::endl;
    std::cout << "  --recovery-component NAME    Label for logs (e.g. iBSS)"
              << std::endl;
    std::cout << "  --recovery-chain             Probe/run iBSS→iBEC→rdsk Recovery chain"
              << std::endl;
    std::cout << "  --recovery-execute           Recovery-only: execute chain uploads (make plugins)"
              << std::endl;
    std::cout << "  --build-ramdisk PATH         Build in-memory HFS+ .dmg from overlay"
              << std::endl;
    std::cout << "  --ramdisk-from-ipsw          Mutate stock RestoreRamDisk → IM4P (needs --ipsw)"
              << std::endl;
    std::cout << "  --ramdisk-overlay DIR        Files merged into ramdisk volume"
              << std::endl;
    std::cout << "  --ramdisk-work-dir DIR       Scratch dir for ipsw extract/repack"
              << std::endl;
    std::cout << "  --ramdisk-size BYTES         Blank build size (default 16m)"
              << std::endl;
    std::cout << "  --ramdisk-label NAME         HFS+ volume label (default purplepois0n)"
              << std::endl;
    std::cout << "  --ramdisk-ident Erase|Update RestoreRamDisk variant (default Erase)"
              << std::endl;
    std::cout << "  --ramdisk-add HOST:/path     Stage host file into HFS+ at build time (repeatable)"
              << std::endl;
    std::cout << "  --ramdisk-add-macho HOST:/path Stage arm64 Mach-O from Mac (repeatable)"
              << std::endl;
    std::cout << "  --ramdisk-probe              Probe booted ramdisk agent (TCP default)"
              << std::endl;
    std::cout << "  --ramdisk-exec CMD           Run shell command on booted ramdisk"
              << std::endl;
    std::cout << "  --ramdisk-push LOCAL REMOTE  Upload file to booted ramdisk"
              << std::endl;
    std::cout << "  --ramdisk-pull REMOTE LOCAL  Download file from booted ramdisk"
              << std::endl;
    std::cout << "  --ramdisk-ls REMOTE          List directory on booted ramdisk"
              << std::endl;
    std::cout << "  --ramdisk-transport tcp|ssh  Live comm transport (default tcp)"
              << std::endl;
    std::cout << "  --ramdisk-tcp-port PORT      Local iproxy/TCP port (default 4444)"
              << std::endl;
    std::cout << "  --ramdisk-device-port PORT   Device-side port for iproxy (default 4444 or 22)"
              << std::endl;
    std::cout << "  --ramdisk-ssh-port PORT      Local iproxy/SSH port when transport=ssh"
              << std::endl;
    std::cout << "  --ramdisk-ssh-pass PASS      SSH password (default alpine)"
              << std::endl;
    std::cout << "  --ramdisk-ssh-key PATH       SSH private key (skips sshpass)"
              << std::endl;
    std::cout << "  --no-ramdisk-iproxy          Do not auto-start iproxy (port already forwarded)"
              << std::endl;
    std::cout << "  --pongo-probe                Probe PongoOS USB shell (05ac:4141; needs libusb)"
              << std::endl;
    std::cout << "  --pongo-boot                 Probe/run KPF + ramdisk boot via Pongo (not Recovery)"
              << std::endl;
    std::cout << "  --pongo-execute              Upload KPF/ramdisk and send bootx (make plugins)"
              << std::endl;
    std::cout << "  --pongo-kpf PATH             checkra1n KPF blob for --pongo-boot"
              << std::endl;
    std::cout << "  --pongo-ramdisk PATH         Raw HFS+ .dmg for Pongo bulk upload"
              << std::endl;
    std::cout << "  --pongo-spawn-checkra1n      Run checkra1n -cp before Pongo probe/boot"
              << std::endl;
    std::cout << "  --fetch-shsh PATH            Live TSS fetch to PATH (libtatsu or ipsw)"
              << std::endl;
    std::cout << "  --sign-macho PATH            Host ad-hoc / cert sign Mach-O (ipsw macho sign)"
              << std::endl;
    std::cout << "  --sign-app PATH              Sign .app bundle directory"
              << std::endl;
    std::cout << "  --sign-ipa PATH              Unpack, sign Payload/*.app, repack IPA"
              << std::endl;
    std::cout << "  --sign-id BUNDLE_ID          Bundle identifier for signing"
              << std::endl;
    std::cout << "  --ent PATH                   Entitlements plist for signing"
              << std::endl;
    std::cout << "  --ad-hoc                     Ad-hoc sign (default for --sign-*)"
              << std::endl;
    std::cout << "  --output PATH                Signed output path"
              << std::endl;
    std::cout << "  --install-ipa PATH           Install IPA via instproxy (requires -d UDID)"
              << std::endl;
    std::cout << "  --trustcache-add PATH        Post-JB trust cache add (jbctl delegate)"
              << std::endl;
    std::cout << "  --post-jb-pipeline           Sign IPA → install → trustcache (make plugins)"
              << std::endl;
    std::cout << "  --futurerestore-restore      Spawn futurerestore (requires --i-understand-restore)"
              << std::endl;
    std::cout << "  --i-understand-restore       Acknowledge destructive restore with futurerestore"
              << std::endl;
    std::cout << std::endl;
    std::cout << "Framework APIs (not all exposed on CLI):" << std::endl;
    std::cout << "  AFCService                 listDirectory / upload / download (Normal mode)"
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
    ctx.jailbreakGeneration = detectJailbreakGeneration(ctx);

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

static bool resolveNormalUdid(DeviceManager& manager, const std::string& targetUDID,
                              std::string& outUdid) {
    if (manager.detectDeviceState(targetUDID) != DeviceState::Normal) {
        Logger::error("AFC requires a trusted device in Normal mode (use -d UDID).");
        return false;
    }
    try {
        auto device = manager.getMobileDevice(targetUDID);
        if (!device) {
            Logger::error("Failed to connect in Normal mode (trust/unlock required).");
            return false;
        }
        outUdid = device->getUDID();
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("Normal mode connect failed: ") + e.what());
        return false;
    }
}

static bool runAfcList(DeviceManager& manager, const std::string& targetUDID,
                       const std::string& remotePath) {
    std::string udid;
    if (!resolveNormalUdid(manager, targetUDID, udid)) {
        return false;
    }
    try {
        AFCService afc(udid);
        const std::vector<AFCFileEntry> entries = afc.listDirectory(remotePath);
        std::cout << remotePath << " (" << entries.size() << " entries):" << std::endl;
        for (size_t i = 0; i < entries.size(); ++i) {
            std::cout << "  " << (entries[i].isDirectory ? "[dir]  " : "[file] ")
                      << entries[i].name << std::endl;
        }
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("AFC list failed: ") + e.what());
        return false;
    }
}

static bool runAfcPush(DeviceManager& manager, const std::string& targetUDID,
                       const std::string& localPath, const std::string& remotePath) {
    std::string udid;
    if (!resolveNormalUdid(manager, targetUDID, udid)) {
        return false;
    }
    try {
        AFCService afc(udid);
        afc.uploadFile(localPath, remotePath);
        Logger::info("Uploaded " + localPath + " -> " + remotePath);
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("AFC push failed: ") + e.what());
        return false;
    }
}

static bool runAfcPull(DeviceManager& manager, const std::string& targetUDID,
                       const std::string& remotePath, const std::string& localPath) {
    std::string udid;
    if (!resolveNormalUdid(manager, targetUDID, udid)) {
        return false;
    }
    try {
        AFCService afc(udid);
        afc.downloadFile(remotePath, localPath);
        Logger::info("Downloaded " + remotePath + " -> " + localPath);
        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("AFC pull failed: ") + e.what());
        return false;
    }
}

int main(int argc, char* argv[]) {
    bool listDevicesFlag = false;
    bool jailbreakFlag = true;
    bool gen0Flag = false;
    bool checkm8Flag = false;
    bool analyzeBackupFlag = false;
    bool analyzeCrashFlag = false;
    bool analyzeBinaryFlag = false;
    bool analyzeDyldCacheFlag = false;
    bool afcListFlag = false;
    bool afcPushFlag = false;
    bool afcPullFlag = false;
    bool tssCheckFlag = false;
    bool fetchShshFlag = false;
    bool signMachoFlag = false;
    bool signAppFlag = false;
    bool signIpaFlag = false;
    bool installIpaFlag = false;
    bool trustCacheAddFlag = false;
    bool adHocSign = true;
    bool verbose = false;
    std::string targetUDID;
    std::string backupPath;
    std::string crashPath;
    std::string binaryPath;
    std::string dyldCachePath;
    std::string analyzeJsonPath;
    std::string reportPath;
    std::string archPreferenceStr;
    std::string afcRemotePath;
    std::string afcLocalPath;
    std::string ipswPath;
    std::string apticketPath;
    FutureRestoreOptions futureRestoreCli;
    std::string im4mManifestPath;
    std::string ipswComponentPath;
    std::string recoveryUploadPath;
    std::string recoveryComponentLabel;
    std::string fetchShshPath;
    std::string signInputPath;
    std::string signOutputPath;
    std::string signBundleId;
    std::string signEntPath;
    std::string installIpaPath;
    std::string trustCachePath;
    CodesignOptions codesignCli;
    std::string buildRamdiskPath;
    bool ramdiskFromIpswFlag = false;
    std::string ramdiskOverlayPath;
    std::string ramdiskWorkDir;
    std::string ramdiskIdent;
    std::string ramdiskSizeStr;
    std::string ramdiskLabel;
    bool recoveryChainFlag = false;
    bool recoveryExecuteFlag = false;
    RamdiskOptions ramdiskCli = ramdiskOptionsFromEnv();
    RamdiskConnectOptions ramdiskConnectCli = ramdiskConnectOptionsFromEnv();
    bool ramdiskProbeFlag = false;
    bool ramdiskExecFlag = false;
    bool ramdiskPushFlag = false;
    bool ramdiskPullFlag = false;
    bool ramdiskListFlag = false;
    std::string ramdiskExecCommand;
    std::string ramdiskPushLocal;
    std::string ramdiskPushRemote;
    std::string ramdiskPullRemote;
    std::string ramdiskPullLocal;
    std::string ramdiskListPath;
    std::string ramdiskSshPortStr;
    std::string ramdiskSshPass;
    std::string ramdiskSshKey;
    bool ramdiskNoIproxy = false;
    std::vector<RamdiskStageEntry> ramdiskStagedFiles;
    std::string ramdiskTransportStr;
    std::string ramdiskTcpPortStr;
    std::string ramdiskDevicePortStr;
    bool pongoProbeFlag = false;
    bool pongoBootFlag = false;
    bool pongoExecuteFlag = false;
    bool pongoSpawnCheckra1nFlag = false;
    std::string pongoKpfPath;
    std::string pongoRamdiskPath;
    std::string pongoXargsLine;
    bool postJbPipelineFlag = false;
    bool futurerestoreRestoreFlag = false;
    bool understandRestoreFlag = false;

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
        } else if (strcmp(argv[i], "--analyze-crash") == 0) {
            if (i + 1 < argc) {
                crashPath = argv[++i];
                analyzeCrashFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--analyze-crash requires a PATH argument");
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
        } else if (strcmp(argv[i], "--afc-list") == 0) {
            if (i + 1 < argc) {
                afcRemotePath = argv[++i];
                afcListFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--afc-list requires a REMOTE_PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--afc-push") == 0) {
            if (i + 2 < argc) {
                afcLocalPath = argv[++i];
                afcRemotePath = argv[++i];
                afcPushFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--afc-push requires LOCAL and REMOTE arguments");
                return 1;
            }
        } else if (strcmp(argv[i], "--afc-pull") == 0) {
            if (i + 2 < argc) {
                afcRemotePath = argv[++i];
                afcLocalPath = argv[++i];
                afcPullFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--afc-pull requires REMOTE and LOCAL arguments");
                return 1;
            }
        } else if (strcmp(argv[i], "--tss-check") == 0) {
            tssCheckFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--ipsw") == 0) {
            if (i + 1 < argc) {
                ipswPath = argv[++i];
            } else {
                Logger::error("--ipsw requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--apticket") == 0) {
            if (i + 1 < argc) {
                apticketPath = argv[++i];
            } else {
                Logger::error("--apticket requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--latest-sep") == 0) {
            futureRestoreCli.latestSep = true;
        } else if (strcmp(argv[i], "--latest-baseband") == 0) {
            futureRestoreCli.latestBaseband = true;
        } else if (strcmp(argv[i], "--no-baseband") == 0) {
            futureRestoreCli.noBaseband = true;
        } else if (strcmp(argv[i], "--sep-ipsw") == 0) {
            if (i + 1 < argc) {
                futureRestoreCli.sepManifestIpsw = argv[++i];
            } else {
                Logger::error("--sep-ipsw requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--bb-ipsw") == 0) {
            if (i + 1 < argc) {
                futureRestoreCli.bbManifestIpsw = argv[++i];
            } else {
                Logger::error("--bb-ipsw requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--im4m-manifest") == 0) {
            if (i + 1 < argc) {
                im4mManifestPath = argv[++i];
            } else {
                Logger::error("--im4m-manifest requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ipsw-component") == 0) {
            if (i + 1 < argc) {
                ipswComponentPath = argv[++i];
            } else {
                Logger::error("--ipsw-component requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--recovery-upload") == 0) {
            if (i + 1 < argc) {
                recoveryUploadPath = argv[++i];
            } else {
                Logger::error("--recovery-upload requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--recovery-component") == 0) {
            if (i + 1 < argc) {
                recoveryComponentLabel = argv[++i];
            } else {
                Logger::error("--recovery-component requires a NAME argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--recovery-chain") == 0) {
            recoveryChainFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--recovery-execute") == 0) {
            recoveryExecuteFlag = true;
            recoveryChainFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--build-ramdisk") == 0) {
            if (i + 1 < argc) {
                buildRamdiskPath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--build-ramdisk requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-from-ipsw") == 0) {
            ramdiskFromIpswFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--ramdisk-overlay") == 0) {
            if (i + 1 < argc) {
                ramdiskOverlayPath = argv[++i];
            } else {
                Logger::error("--ramdisk-overlay requires a DIR argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-work-dir") == 0) {
            if (i + 1 < argc) {
                ramdiskWorkDir = argv[++i];
            } else {
                Logger::error("--ramdisk-work-dir requires a DIR argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-size") == 0) {
            if (i + 1 < argc) {
                ramdiskSizeStr = argv[++i];
            } else {
                Logger::error("--ramdisk-size requires a BYTES argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-label") == 0) {
            if (i + 1 < argc) {
                ramdiskLabel = argv[++i];
            } else {
                Logger::error("--ramdisk-label requires a NAME argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-ident") == 0) {
            if (i + 1 < argc) {
                ramdiskIdent = argv[++i];
            } else {
                Logger::error("--ramdisk-ident requires Erase or Update");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-add") == 0) {
            if (i + 1 < argc) {
                RamdiskStageEntry entry;
                if (!parseRamdiskAddSpec(argv[++i], &entry)) {
                    Logger::error("--ramdisk-add requires HOST:/hfs/path (e.g. ./agent:/sbin/agent)");
                    return 1;
                }
                ramdiskStagedFiles.push_back(entry);
            } else {
                Logger::error("--ramdisk-add requires HOST:/hfs/path");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-add-macho") == 0) {
            if (i + 1 < argc) {
                RamdiskStageEntry entry;
                if (!parseRamdiskAddSpec(argv[++i], &entry)) {
                    Logger::error("--ramdisk-add-macho requires HOST:/hfs/path");
                    return 1;
                }
                entry.verifyMachOArm64 = true;
                ramdiskStagedFiles.push_back(entry);
            } else {
                Logger::error("--ramdisk-add-macho requires HOST:/hfs/path");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-transport") == 0) {
            if (i + 1 < argc) {
                ramdiskTransportStr = argv[++i];
            } else {
                Logger::error("--ramdisk-transport requires tcp or ssh");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-tcp-port") == 0) {
            if (i + 1 < argc) {
                ramdiskTcpPortStr = argv[++i];
            } else {
                Logger::error("--ramdisk-tcp-port requires a PORT argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-device-port") == 0) {
            if (i + 1 < argc) {
                ramdiskDevicePortStr = argv[++i];
            } else {
                Logger::error("--ramdisk-device-port requires a PORT argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-probe") == 0) {
            ramdiskProbeFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--ramdisk-exec") == 0) {
            if (i + 1 < argc) {
                ramdiskExecCommand = argv[++i];
                ramdiskExecFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--ramdisk-exec requires a CMD argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-push") == 0) {
            if (i + 2 < argc) {
                ramdiskPushLocal = argv[++i];
                ramdiskPushRemote = argv[++i];
                ramdiskPushFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--ramdisk-push requires LOCAL and REMOTE arguments");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-pull") == 0) {
            if (i + 2 < argc) {
                ramdiskPullRemote = argv[++i];
                ramdiskPullLocal = argv[++i];
                ramdiskPullFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--ramdisk-pull requires REMOTE and LOCAL arguments");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-ls") == 0) {
            if (i + 1 < argc) {
                ramdiskListPath = argv[++i];
                ramdiskListFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--ramdisk-ls requires a REMOTE path argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-ssh-port") == 0) {
            if (i + 1 < argc) {
                ramdiskSshPortStr = argv[++i];
            } else {
                Logger::error("--ramdisk-ssh-port requires a PORT argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-ssh-pass") == 0) {
            if (i + 1 < argc) {
                ramdiskSshPass = argv[++i];
            } else {
                Logger::error("--ramdisk-ssh-pass requires a PASS argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-ssh-key") == 0) {
            if (i + 1 < argc) {
                ramdiskSshKey = argv[++i];
            } else {
                Logger::error("--ramdisk-ssh-key requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--no-ramdisk-iproxy") == 0) {
            ramdiskNoIproxy = true;
        } else if (strcmp(argv[i], "--pongo-probe") == 0) {
            pongoProbeFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--pongo-boot") == 0) {
            pongoBootFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--pongo-execute") == 0) {
            pongoExecuteFlag = true;
            pongoBootFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--pongo-kpf") == 0) {
            if (i + 1 < argc) {
                pongoKpfPath = argv[++i];
            } else {
                Logger::error("--pongo-kpf requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--pongo-ramdisk") == 0) {
            if (i + 1 < argc) {
                pongoRamdiskPath = argv[++i];
            } else {
                Logger::error("--pongo-ramdisk requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--pongo-spawn-checkra1n") == 0) {
            pongoSpawnCheckra1nFlag = true;
        } else if (strcmp(argv[i], "--fetch-shsh") == 0) {
            if (i + 1 < argc) {
                fetchShshPath = argv[++i];
                fetchShshFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--fetch-shsh requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--sign-macho") == 0) {
            if (i + 1 < argc) {
                signInputPath = argv[++i];
                signMachoFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--sign-macho requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--sign-app") == 0) {
            if (i + 1 < argc) {
                signInputPath = argv[++i];
                signAppFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--sign-app requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--sign-ipa") == 0) {
            if (i + 1 < argc) {
                signInputPath = argv[++i];
                signIpaFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--sign-ipa requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--sign-id") == 0) {
            if (i + 1 < argc) {
                signBundleId = argv[++i];
            } else {
                Logger::error("--sign-id requires a BUNDLE_ID argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ent") == 0) {
            if (i + 1 < argc) {
                signEntPath = argv[++i];
            } else {
                Logger::error("--ent requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--ad-hoc") == 0) {
            adHocSign = true;
        } else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                signOutputPath = argv[++i];
            } else {
                Logger::error("--output requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--install-ipa") == 0) {
            if (i + 1 < argc) {
                installIpaPath = argv[++i];
                installIpaFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--install-ipa requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--trustcache-add") == 0) {
            if (i + 1 < argc) {
                trustCachePath = argv[++i];
                trustCacheAddFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--trustcache-add requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--post-jb-pipeline") == 0) {
            postJbPipelineFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--futurerestore-restore") == 0) {
            futurerestoreRestoreFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--i-understand-restore") == 0) {
            understandRestoreFlag = true;
        } else {
            Logger::error("Unknown option: " + std::string(argv[i]));
            printUsage(argv[0]);
            return 1;
        }
    }
    
    (void)verbose;

    if (!ramdiskSizeStr.empty()) {
        uint64_t bytes = 0;
        if (!parseRamdiskSize(ramdiskSizeStr, &bytes)) {
            Logger::error("Invalid --ramdisk-size: " + ramdiskSizeStr);
            return 1;
        }
        ramdiskCli.sizeBytes = bytes;
    }
    if (!ramdiskLabel.empty()) {
        ramdiskCli.volumeLabel = ramdiskLabel;
    }
    if (!ramdiskTransportStr.empty()) {
        ramdiskConnectCli.transport = ramdiskTransportFromString(ramdiskTransportStr);
    }
    if (!ramdiskTcpPortStr.empty()) {
        const long port = std::strtol(ramdiskTcpPortStr.c_str(), nullptr, 10);
        if (port > 0 && port <= 65535) {
            ramdiskConnectCli.tcpPort = static_cast<uint16_t>(port);
        }
    }
    if (!ramdiskDevicePortStr.empty()) {
        const long port = std::strtol(ramdiskDevicePortStr.c_str(), nullptr, 10);
        if (port > 0 && port <= 65535) {
            ramdiskConnectCli.deviceTcpPort = static_cast<uint16_t>(port);
            ramdiskConnectCli.deviceSshPort = static_cast<uint16_t>(port);
        }
    }
    if (!ramdiskSshPortStr.empty()) {
        const long port = std::strtol(ramdiskSshPortStr.c_str(), nullptr, 10);
        if (port > 0 && port <= 65535) {
            ramdiskConnectCli.sshPort = static_cast<uint16_t>(port);
        }
    }
    if (!ramdiskSshPass.empty()) {
        ramdiskConnectCli.sshPassword = ramdiskSshPass;
    }
    if (!ramdiskSshKey.empty()) {
        ramdiskConnectCli.sshKeyPath = ramdiskSshKey;
    }
    if (ramdiskNoIproxy) {
        ramdiskConnectCli.autoIproxy = false;
    }
    if (!targetUDID.empty()) {
        ramdiskConnectCli.udid = targetUDID;
    }

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

    if (analyzeCrashFlag) {
        return analyzeCrashLog(crashPath) ? 0 : 1;
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

    if (afcListFlag) {
        if (targetUDID.empty()) {
            Logger::error("--afc-list requires -d UDID");
            return 1;
        }
        return runAfcList(manager, targetUDID, afcRemotePath) ? 0 : 1;
    }

    if (afcPushFlag) {
        if (targetUDID.empty()) {
            Logger::error("--afc-push requires -d UDID");
            return 1;
        }
        return runAfcPush(manager, targetUDID, afcLocalPath, afcRemotePath) ? 0 : 1;
    }

    if (afcPullFlag) {
        if (targetUDID.empty()) {
            Logger::error("--afc-pull requires -d UDID");
            return 1;
        }
        return runAfcPull(manager, targetUDID, afcRemotePath, afcLocalPath) ? 0 : 1;
    }

    if (tssCheckFlag) {
        return runTssCheck(manager, targetUDID, "", "", 0, ipswPath, apticketPath) ? 0 : 1;
    }

    if (fetchShshFlag) {
        if (ipswPath.empty()) {
            Logger::warn("--fetch-shsh without --ipsw uses ipsw download tss only (Normal/USB metadata)");
        }
        return fetchLiveShsh(manager, targetUDID, ipswPath, fetchShshPath) ? 0 : 1;
    }

    if (!buildRamdiskPath.empty()) {
        return runBuildRamdisk(ramdiskCli, ramdiskOverlayPath, ramdiskStagedFiles, buildRamdiskPath)
                   ? 0
                   : 1;
    }

    if (ramdiskFromIpswFlag) {
        if (ipswPath.empty()) {
            Logger::error("--ramdisk-from-ipsw requires --ipsw");
            return 1;
        }
        return runRamdiskFromIpsw(ramdiskCli, ipswPath, ramdiskIdent, ramdiskOverlayPath,
                                  ramdiskStagedFiles, ramdiskWorkDir, signOutputPath)
                   ? 0
                   : 1;
    }

    if (ramdiskProbeFlag) {
        std::string message;
        return runRamdiskProbe(ramdiskConnectCli, &message) ? 0 : 1;
    }
    if (ramdiskExecFlag) {
        return runRamdiskExec(ramdiskConnectCli, ramdiskExecCommand) ? 0 : 1;
    }
    if (ramdiskPushFlag) {
        return runRamdiskPush(ramdiskConnectCli, ramdiskPushLocal, ramdiskPushRemote) ? 0 : 1;
    }
    if (ramdiskPullFlag) {
        return runRamdiskPull(ramdiskConnectCli, ramdiskPullRemote, ramdiskPullLocal) ? 0 : 1;
    }
    if (ramdiskListFlag) {
        return runRamdiskList(ramdiskConnectCli, ramdiskListPath) ? 0 : 1;
    }

    if (pongoProbeFlag) {
        std::string message;
        return runPongoProbe(pongoSpawnCheckra1nFlag, false, &message) ? 0 : 1;
    }

    auto buildCliParsed = [&]() -> CliParsedOptions {
        CliParsedOptions cli;
        cli.reportPath = reportPath;
        cli.backupPath = backupPath;
        cli.ipswPath = ipswPath;
        cli.apticketPath = apticketPath;
        cli.futureRestore = futureRestoreCli;
        cli.im4mManifestPath = im4mManifestPath;
        cli.ipswComponentPath = ipswComponentPath;
        cli.recoveryUploadPath = recoveryUploadPath;
        cli.recoveryComponentLabel = recoveryComponentLabel;
        cli.codesign = codesignCli;
        cli.ipaInstallPath = installIpaPath;
        cli.trustCachePath = trustCachePath;
        cli.buildRamdiskPath = buildRamdiskPath;
        cli.ramdiskFromIpswOutput = signOutputPath;
        cli.ramdiskOverlayPath = ramdiskOverlayPath;
        cli.ramdiskWorkDir = ramdiskWorkDir;
        cli.ramdiskIdent = ramdiskIdent;
        cli.ramdiskBuild = ramdiskCli;
        cli.ramdiskStagedFiles = ramdiskStagedFiles;
        cli.ramdiskConnect = ramdiskConnectCli;
        cli.ramdiskExecCommand = ramdiskExecCommand;
        cli.ramdiskUploadLocal = ramdiskPushLocal;
        cli.ramdiskUploadRemote = ramdiskPushRemote;
        cli.ramdiskDownloadRemote = ramdiskPullRemote;
        cli.ramdiskDownloadLocal = ramdiskPullLocal;
        cli.ramdiskListPath = ramdiskListPath;
        cli.recoveryChainFlag = recoveryChainFlag;
        cli.recoveryExecuteFlag = recoveryExecuteFlag;
        cli.pongoProbeFlag = pongoProbeFlag;
        cli.pongoBootFlag = pongoBootFlag;
        cli.pongoExecuteFlag = pongoExecuteFlag;
        cli.pongoSpawnCheckra1nFlag = pongoSpawnCheckra1nFlag;
        cli.pongoKpfPath = pongoKpfPath;
        cli.pongoRamdiskPath = pongoRamdiskPath;
        cli.pongoXargsLine = pongoXargsLine;
        cli.postJbPipelineFlag = postJbPipelineFlag;
        cli.futurerestoreRestoreFlag = futurerestoreRestoreFlag;
        if (signMachoFlag || signAppFlag || signIpaFlag) {
            cli.codesignInputPath = signInputPath;
            cli.codesignOutputPath = signOutputPath;
        }
        return cli;
    };

    if (pongoBootFlag) {
        return runPongoBoot(gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::PongoBoot),
                            pongoExecuteFlag)
                   ? 0
                   : 1;
    }

    if (futurerestoreRestoreFlag) {
        if (!understandRestoreFlag) {
            Logger::error("futurerestore restore requires --i-understand-restore");
            return 1;
        }
        if (!exploitPluginsEnabled()) {
            Logger::error("futurerestore restore requires make plugins");
            return 1;
        }
        Gen0Options frOpts = gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0);
        return runFuturerestoreRestore(frOpts, true) ? 0 : 1;
    }

    codesignCli.bundleId = signBundleId;
    codesignCli.entitlementsPath = signEntPath;
    codesignCli.outputPath = signOutputPath;
    codesignCli.adHoc = adHocSign;

    if (postJbPipelineFlag && !gen0Flag) {
        if (!exploitPluginsEnabled()) {
            Logger::error("Post-jb pipeline requires make plugins");
            return 1;
        }
        Gen0Options pipeOpts = gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0);
        if (signIpaFlag) {
            pipeOpts.codesignInputPath = signInputPath;
            pipeOpts.codesignOutputPath = signOutputPath;
            pipeOpts.codesign = codesignCli;
        }
        pipeOpts.ipaInstallPath = installIpaPath;
        pipeOpts.trustCachePath = trustCachePath;
        pipeOpts.ramdisk.connect = ramdiskConnectCli;
        if (pipeOpts.ramdisk.connect.udid.empty() && !targetUDID.empty()) {
            pipeOpts.ramdisk.connect.udid = targetUDID;
        }
        return runPostJbPipeline(manager, targetUDID, pipeOpts) ? 0 : 1;
    }

    if (!gen0Flag) {
        if (signMachoFlag || signAppFlag) {
            return runHostCodesign(signInputPath, codesignCli, true) ? 0 : 1;
        }
        if (signIpaFlag) {
            return runHostSignIpa(signInputPath, signOutputPath, codesignCli, true) ? 0 : 1;
        }
        if (installIpaFlag) {
            return runSideloadInstall(manager, targetUDID, installIpaPath,
                                      exploitPluginsEnabled())
                       ? 0
                       : 1;
        }
        if (trustCacheAddFlag) {
            return runTrustCacheAdd(trustCachePath, exploitPluginsEnabled()) ? 0 : 1;
        }
    }

    if (!targetUDID.empty() && ramdiskConnectCli.udid.empty()) {
        ramdiskConnectCli.udid = targetUDID;
    }

    if (gen0Flag) {
        return runGen0Jailbreak(manager, targetUDID,
                                gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0))
                   ? 0
                   : 1;
    }

    if (recoveryChainFlag || recoveryExecuteFlag) {
        return runGen0Jailbreak(manager, targetUDID,
                                gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::RecoveryChain))
                   ? 0
                   : 1;
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
