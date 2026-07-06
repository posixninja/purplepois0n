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
#include <sstream>
#include "DeviceManager.h"
#include "Gen0Workflow.h"
#include "Gen0CliOptions.h"
#include "DoctorWorkflow.h"
#include "JailbreakPlanner.h"
#include "Checkm8.h"
#include "Usbliter8.h"
#include "BootromExploit.h"
#include "Checkm8Exploit.h"
#include "Usbliter8Exploit.h"
#include "AnthraxExploit.h"
#include "Logger.h"
#include "MachOParser.h"
#include "DFUDevice.h"
#include "AFCService.h"
#include "MobileDevice.h"
#include "primitives/ChainRunner.h"
#include "primitives/DfuTransport.h"
#include "primitives/PrimitiveTypes.h"
#include "primitives/Gen6Types.h"
#include "primitives/PrimitiveRegistry.h"
#include "primitives/TssTypes.h"
#include "primitives/CodesignTypes.h"
#include "RamdiskTypes.h"
#include "EnvUtil.h"
#include "cli/CliParser.h"
#include "pongo/PongoBootHelpers.h"
#include "../include/DeviceState.h"

#include <sys/stat.h>

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
    std::cout << "  --dfu-jailbreak            checkm8 + Pongo KPF/ramdisk (make plugins + --i-understand-jailbreak)"
              << std::endl;
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
    std::cout << "  --update                   futurerestore/idevicerestore: OTA-style update install"
              << std::endl;
    std::cout << "  --wait                     futurerestore: ApNonce collision loop (-w)"
              << std::endl;
    std::cout << "  --use-pwndfu               futurerestore: Odysseus restore from pwned DFU"
              << std::endl;
    std::cout << "  --just-boot                futurerestore: tethered boot only (no full restore)"
              << std::endl;
    std::cout << "  --just-boot-args LINE      boot-args after --just-boot (e.g. \"-v\")"
              << std::endl;
    std::cout << "  --exit-recovery            futurerestore: exit recovery and quit (-e)"
              << std::endl;
    std::cout << "  --debug-restore            Verbose futurerestore/idevicerestore logging"
              << std::endl;
    std::cout << "  --sep PATH                 futurerestore: manual SEP.im4p (-s)"
              << std::endl;
    std::cout << "  --sep-manifest PATH        futurerestore: SEP BuildManifest (-m)"
              << std::endl;
    std::cout << "  --baseband PATH            futurerestore: manual baseband (-b)"
              << std::endl;
    std::cout << "  --baseband-manifest PATH   futurerestore: baseband BuildManifest (-p)"
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
    std::cout << "  --ramdisk PATH               Boot artifact (.dmg raw or .im4p; lane-specific)"
              << std::endl;
    std::cout << "  --boot-lane LANE             auto|recovery|usb-loader|post-exploit|live-agent"
              << std::endl;
    std::cout << "  --ramdisk-format FMT         auto|raw-dmg|im4p"
              << std::endl;
    std::cout << "  --boot-module PATH           Post-loader module (KPF; aliases --pongo-kpf)"
              << std::endl;
    std::cout << "  --boot-args LINE             Kernel boot-args (Pongo xargs; env PURPLEPOIS0N_PONGO_XARGS)"
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
    std::cout << "  --pongo-kpf PATH             KPF blob for --pongo-boot (or PURPLEPOIS0N_KPF / built module)"
              << std::endl;
    std::cout << "  --pongo-ramdisk PATH         (alias) same as --ramdisk for usb-loader lane"
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
    std::cout << "  --medicine-probe             Post-JB cures plan (hacktivation / AFC2 / capable)"
              << std::endl;
    std::cout << "  --medicine-apply             Apply medicine cures over SSH (make plugins)"
              << std::endl;
    std::cout << "  --medicine                   Include medicine step in --post-jb-pipeline"
              << std::endl;
    std::cout << "  --medicine-cures LIST        afc2,capable,sachet,loader,all (default: afc2,capable,loader)"
              << std::endl;
    std::cout << "  --medicine-platform TYPE     SpringBoard platform plist (e.g. iPhone4,1)"
              << std::endl;
    std::cout << "  --medicine-capability KEY    Capability to strip (capable.mm; default: wifi)"
              << std::endl;
    std::cout << "  --medicine-app PATH          App bundle path for sachet register"
              << std::endl;
    std::cout << "  --futurerestore-restore      Spawn futurerestore (requires --i-understand-restore)"
              << std::endl;
    std::cout << "  --idevicerestore-restore     Spawn idevicerestore stock restore (same ack)"
              << std::endl;
    std::cout << "  --i-understand-restore       Acknowledge destructive restore with futurerestore"
              << std::endl;
    std::cout << "  --jailbreak-execute          Run era execute chain (requires make plugins)"
              << std::endl;
    std::cout << "  --i-understand-jailbreak     Acknowledge mutating jailbreak execute path"
              << std::endl;
    std::cout << "  --doctor-run                 Scan device, plan strategy, run jailbreak (JSON steps)"
              << std::endl;
    std::cout << "  --device-plan                JSON device profile + jailbreak plan (probe only)"
              << std::endl;
    std::cout << "  --capabilities               Print JSON host capabilities (plugins, store)"
              << std::endl;
    std::cout << std::endl;
    std::cout << "DFU jailbreak recipe (A5–A11, device in DFU):" << std::endl;
    std::cout << "  make plugins kpf" << std::endl;
    std::cout << "  ./purplepois0n --dfu-jailbreak --i-understand-jailbreak \\" << std::endl;
    std::cout << "    --boot-lane usb-loader \\" << std::endl;
    std::cout << "    --boot-module legacy/kpf-purple/build/purplepois0n-kpf-pongo \\" << std::endl;
    std::cout << "    --ramdisk /path/to/raw.dmg   # or --ipsw firmware.ipsw" << std::endl;
    std::cout << "  --kernelcache PATH           Host kernelcache for offline patchfind/patch"
              << std::endl;
    std::cout << "  --patch-profile PATH         JSON patch descriptors (offset + hex/bytes)"
              << std::endl;
    std::cout << "  --patch-out PATH             Output path for patched kernelcache"
              << std::endl;
    std::cout << "  --normal-ssh                 Use SSH for trustcache on jailbroken Normal device"
              << std::endl;
    std::cout << "  --external-jailbreak         Delegate to palera1n/checkra1n script (make plugins)"
              << std::endl;
    std::cout << "  --already-jailbroken         With --external-jailbreak: probe /var/jb only, skip helper"
              << std::endl;
    std::cout << "  --rootless-probe             Probe /var/jb bootstrap over SSH (needs --normal-ssh)"
              << std::endl;
    std::cout << "  --store-init                 Create host dpkg repo under store/ (or PURPLEPOIS0N_STORE_ROOT)"
              << std::endl;
    std::cout << "  --store-build                Regenerate Packages/Release from pool/*.deb"
              << std::endl;
    std::cout << "  --store-add PATH             Copy .deb into pool and rebuild index"
              << std::endl;
    std::cout << "  --store-sync                 Push repo to device (needs --normal-ssh)"
              << std::endl;
    std::cout << "  --store-sync-mode MODE       file (default), https, or sources-only"
              << std::endl;
    std::cout << "  --store-installed            List installed packages on device (needs --normal-ssh)"
              << std::endl;
    std::cout << "  --store-install PKG          apt/dpkg install package after sync"
              << std::endl;
    std::cout << "  --store-publish [PATH]       Export repo tree for HTTPS hosting (default: store-publish)"
              << std::endl;
    std::cout << "  --store-root PATH            Override host repo root (default: ./store)"
              << std::endl;
    std::cout << "  --post-jb-store              Sync purplepois0n-store during --post-jb-pipeline"
              << std::endl;
    std::cout << "  --post-jb-store-install PKG  apt install package after post-jb store sync"
              << std::endl;
    std::cout << "  --dtree-mmio PATH            Build MMIO catalog from IPSW/DeviceTree (ipsw dtree -j)"
              << std::endl;
    std::cout << "  --dtree-mmio-out PATH        Write MMIO catalog JSON for PhysicalUAF / dmaFail probes"
              << std::endl;
    std::cout << "  --dtree-mmio-all             Include all MMIO regions (default: AGX/ANE/arm-io only)"
              << std::endl;
    std::cout << "  --dtree-registers PATH       Full hardware register inventory from DeviceTree"
              << std::endl;
    std::cout << "  --dtree-registers-out PATH   Write full register catalog JSON"
              << std::endl;
    std::cout << "  --dtree-registers-verbose    Log every register entry"
              << std::endl;
    std::cout << "  --hypervisor-probe PATH      SPTM/hypervisor page-monitor profile (IPSW or DT JSON)"
              << std::endl;
    std::cout << "  --hypervisor-probe-out PATH  Write page-monitor control plan JSON"
              << std::endl;
    std::cout << "  --integrity-probe PATH       PAC + data-integrity (MIE) bypass plan"
              << std::endl;
    std::cout << "  --integrity-probe-out PATH   Write integrity bypass plan JSON"
              << std::endl;
    std::cout << "  --bypass-integrity           Auto-run badRecovery PAC bypass (make plugins)"
              << std::endl;
    std::cout << "  --probe-primitive NAME       Run a single built-in primitive (offline OK)"
              << std::endl;
    std::cout << "  Env: PURPLEPOIS0N_JBROOT, PURPLEPOIS0N_ROOTLESS, PURPLEPOIS0N_JB_HELPER,"
              << std::endl;
    std::cout << "       PURPLEPOIS0N_PALERA1N_HELPER, PURPLEPOIS0N_JBROOT_FIXTURE (offline tree),"
              << std::endl;
    std::cout << "       PURPLEPOIS0N_STORE_ROOT, PURPLEPOIS0N_MMIO_CATALOG (exported catalog JSON),"
              << std::endl;
    std::cout << "       PURPLEPOIS0N_IPSW / PURPLEPOIS0N_DTREE_INPUT (devicetree-mmio primitive)"
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

static bool isRegularFile(const std::string& path) {
    struct stat st = {};
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

static void initBootromExploits() {
    auto& registry = BootromExploitRegistry::instance();
    registry.registerExploit(std::make_shared<Checkm8Exploit>());
    registry.registerExploit(std::make_shared<Usbliter8Exploit>());
    if (envFlagEnabled("PURPLEPOIS0N_ANTHRAX")) {
        registry.registerPostExploit(std::make_shared<AnthraxExploit>());
    }
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
            std::cout << "  CPID: 0x" << std::hex << std::uppercase << device.cpid << std::dec
                      << " (" << Checkm8::cpidToSocName(device.cpid) << ")" << std::endl;
        }
        std::cout << std::endl;
    }
}

static bool performJailbreak(DeviceManager& manager,
                             const std::string& targetUDID,
                             const std::string& reportPath,
                             const Gen0Options& options) {
    Logger::info("Starting jailbreak process...");
    const DeviceState state = manager.detectDeviceState(targetUDID);
    if (state == DeviceState::DFU) {
        Gen0Options dfuOpts = options;
        dfuOpts.reportPath = reportPath;
        if (options.jailbreakExecute) {
            Logger::info("DFU mode — bootrom pwn + Pongo boot chain");
            return runDfuJailbreak(manager, dfuOpts, true);
        }
        Logger::info("DFU mode — running primitive probe chain (no auto-pwn)");
        return runDfuJailbreak(manager, dfuOpts, false);
    }
    Logger::info("Non-DFU mode — Gen 0 scaffold");
    Gen0Options jbOptions = options;
    jbOptions.reportPath = reportPath;
    return runGen0Jailbreak(manager, targetUDID, jbOptions);
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
    initBootromExploits();

    const cli::SubcommandResult sub = cli::trySubcommand(argc, argv);
    if (sub.handled) {
        if (sub.showSubcommandHelp) {
            if (!sub.subcommandHelpTopic.empty()) {
                cli::printTopicHelp(argv[0], sub.subcommandHelpTopic.c_str());
            } else {
                cli::printSubcommandHelp(argv[0]);
            }
            return sub.exitCode;
        }
        return sub.exitCode;
    }

    std::vector<std::string> ownedArgvStorage;
    std::vector<char*> ownedArgvPtrs;
    if (sub.useLegacy && !sub.rewrittenArgv.empty()) {
        ownedArgvStorage = sub.rewrittenArgv;
        ownedArgvPtrs.reserve(ownedArgvStorage.size());
        for (size_t i = 0; i < ownedArgvStorage.size(); ++i) {
            ownedArgvPtrs.push_back(&ownedArgvStorage[i][0]);
        }
        argc = static_cast<int>(ownedArgvPtrs.size());
        argv = ownedArgvPtrs.data();
    }

    bool listDevicesFlag = false;
    bool jailbreakFlag = true;
    bool gen0Flag = false;
    bool checkm8Flag = false;
    bool dfuJailbreakFlag = false;
    bool externalJailbreakFlag = false;
    bool externalSkipHelperFlag = false;
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
    primitives::IdeviceRestoreOptions ideviceRestoreCli;
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
    std::string ramdiskPath;
    std::string bootLaneStr;
    std::string ramdiskFormatStr;
    std::string bootModulePath;
    std::string bootArgsLine;
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
    bool postJbStoreFlag = false;
    std::string postJbStoreInstallPkg;
    bool medicineProbeFlag = false;
    bool medicineApplyFlag = false;
    bool medicineWithPipelineFlag = false;
    std::string medicineCures;
    std::string medicinePlatform;
    std::string medicineCapability;
    std::string medicineAppPath;
    bool futurerestoreRestoreFlag = false;
    bool idevicerestoreRestoreFlag = false;
    bool understandRestoreFlag = false;
    bool jailbreakExecuteFlag = false;
    bool understandJailbreakFlag = false;
    bool doctorRunFlag = false;
    bool devicePlanFlag = false;
    bool capabilitiesFlag = false;
    bool normalSshFlag = false;
    bool rootlessProbeFlag = false;
    bool storeInitFlag = false;
    bool storeBuildFlag = false;
    bool storeSyncFlag = false;
    bool storeInstalledFlag = false;
    std::string storeSyncModeStr = "file";
    bool storePublishFlag = false;
    std::string storePublishPath;
    std::string storeAddPath;
    std::string storeInstallPkg;
    std::string storeRootPath;
    std::string dtreeMmioPath;
    std::string dtreeMmioOutPath;
    bool dtreeMmioAllFlag = false;
    std::string dtreeRegistersPath;
    std::string dtreeRegistersOutPath;
    bool dtreeRegistersVerboseFlag = false;
    std::string hypervisorProbePath;
    std::string hypervisorProbeOutPath;
    std::string integrityProbePath;
    std::string integrityProbeOutPath;
    bool bypassIntegrityFlag = false;
    std::string probePrimitiveName;
    std::string kernelcachePath;
    std::string patchProfilePath;
    std::string patchOutPath;

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
        } else if (strcmp(argv[i], "--external-jailbreak") == 0) {
            externalJailbreakFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--already-jailbroken") == 0) {
            externalSkipHelperFlag = true;
        } else if (strcmp(argv[i], "--dfu-jailbreak") == 0) {
            dfuJailbreakFlag = true;
            jailbreakFlag = false;
            checkm8Flag = false;
            jailbreakExecuteFlag = true;
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
                const std::string ticketPath = argv[++i];
                if (apticketPath.empty()) {
                    apticketPath = ticketPath;
                } else {
                    futureRestoreCli.extraApticketPaths.push_back(ticketPath);
                }
            } else {
                Logger::error("--apticket requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--update") == 0) {
            futureRestoreCli.updateInstall = true;
            ideviceRestoreCli.updateInstall = true;
        } else if (strcmp(argv[i], "--wait") == 0 || strcmp(argv[i], "-w") == 0) {
            futureRestoreCli.waitApNonce = true;
        } else if (strcmp(argv[i], "--use-pwndfu") == 0) {
            futureRestoreCli.usePwndfu = true;
        } else if (strcmp(argv[i], "--just-boot") == 0) {
            futureRestoreCli.justBoot = true;
        } else if (strcmp(argv[i], "--just-boot-args") == 0) {
            if (i + 1 < argc) {
                futureRestoreCli.justBootArgs = argv[++i];
            } else {
                Logger::error("--just-boot-args requires a LINE argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--exit-recovery") == 0 || strcmp(argv[i], "-e") == 0) {
            futureRestoreCli.exitRecovery = true;
        } else if (strcmp(argv[i], "--debug-restore") == 0) {
            futureRestoreCli.debug = true;
            ideviceRestoreCli.debug = true;
        } else if (strcmp(argv[i], "--sep") == 0) {
            if (i + 1 < argc) {
                futureRestoreCli.sepPath = argv[++i];
            } else {
                Logger::error("--sep requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--sep-manifest") == 0) {
            if (i + 1 < argc) {
                futureRestoreCli.sepManifestPath = argv[++i];
            } else {
                Logger::error("--sep-manifest requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--baseband") == 0) {
            if (i + 1 < argc) {
                futureRestoreCli.basebandPath = argv[++i];
            } else {
                Logger::error("--baseband requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--baseband-manifest") == 0) {
            if (i + 1 < argc) {
                futureRestoreCli.basebandManifestPath = argv[++i];
            } else {
                Logger::error("--baseband-manifest requires a PATH argument");
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
        } else if (strcmp(argv[i], "--ramdisk") == 0) {
            if (i + 1 < argc) {
                ramdiskPath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--ramdisk requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--boot-lane") == 0) {
            if (i + 1 < argc) {
                bootLaneStr = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--boot-lane requires LANE (recovery|usb-loader|post-exploit|...)");
                return 1;
            }
        } else if (strcmp(argv[i], "--ramdisk-format") == 0) {
            if (i + 1 < argc) {
                ramdiskFormatStr = argv[++i];
            } else {
                Logger::error("--ramdisk-format requires auto|raw-dmg|im4p");
                return 1;
            }
        } else if (strcmp(argv[i], "--boot-module") == 0) {
            if (i + 1 < argc) {
                bootModulePath = argv[++i];
            } else {
                Logger::error("--boot-module requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--boot-args") == 0 || strcmp(argv[i], "--pongo-xargs") == 0) {
            if (i + 1 < argc) {
                bootArgsLine = argv[++i];
            } else {
                Logger::error("--boot-args requires a LINE argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--pongo-kpf") == 0) {
            if (i + 1 < argc) {
                pongoKpfPath = argv[++i];
                if (bootModulePath.empty()) {
                    bootModulePath = pongoKpfPath;
                }
            } else {
                Logger::error("--pongo-kpf requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--pongo-ramdisk") == 0) {
            if (i + 1 < argc) {
                pongoRamdiskPath = argv[++i];
                if (ramdiskPath.empty()) {
                    ramdiskPath = pongoRamdiskPath;
                }
                if (bootLaneStr.empty()) {
                    bootLaneStr = "usb-loader";
                }
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
        } else if (strcmp(argv[i], "--post-jb-store") == 0) {
            postJbStoreFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--post-jb-store-install") == 0) {
            if (i + 1 < argc) {
                postJbStoreInstallPkg = argv[++i];
                postJbStoreFlag = true;
                jailbreakFlag = false;
            } else {
                Logger::error("--post-jb-store-install requires a PACKAGE argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--medicine-probe") == 0) {
            medicineProbeFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--medicine-apply") == 0) {
            medicineApplyFlag = true;
            medicineProbeFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--medicine") == 0) {
            medicineWithPipelineFlag = true;
            medicineProbeFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--medicine-cures") == 0) {
            if (i + 1 < argc) {
                medicineCures = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--medicine-cures requires a comma-separated list");
                return 1;
            }
        } else if (strcmp(argv[i], "--medicine-platform") == 0) {
            if (i + 1 < argc) {
                medicinePlatform = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--medicine-platform requires TYPE");
                return 1;
            }
        } else if (strcmp(argv[i], "--medicine-capability") == 0) {
            if (i + 1 < argc) {
                medicineCapability = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--medicine-capability requires KEY");
                return 1;
            }
        } else if (strcmp(argv[i], "--medicine-app") == 0) {
            if (i + 1 < argc) {
                medicineAppPath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--medicine-app requires PATH");
                return 1;
            }
        } else if (strcmp(argv[i], "--futurerestore-restore") == 0) {
            futurerestoreRestoreFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--idevicerestore-restore") == 0) {
            idevicerestoreRestoreFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--i-understand-restore") == 0) {
            understandRestoreFlag = true;
        } else if (strcmp(argv[i], "--jailbreak-execute") == 0) {
            jailbreakExecuteFlag = true;
        } else if (strcmp(argv[i], "--i-understand-jailbreak") == 0) {
            understandJailbreakFlag = true;
        } else if (strcmp(argv[i], "--capabilities") == 0) {
            capabilitiesFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--doctor-run") == 0) {
            doctorRunFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--device-plan") == 0) {
            devicePlanFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--kernelcache") == 0) {
            if (i + 1 < argc) {
                kernelcachePath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--kernelcache requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--patch-profile") == 0) {
            if (i + 1 < argc) {
                patchProfilePath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--patch-profile requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--patch-out") == 0) {
            if (i + 1 < argc) {
                patchOutPath = argv[++i];
            } else {
                Logger::error("--patch-out requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--normal-ssh") == 0) {
            normalSshFlag = true;
        } else if (strcmp(argv[i], "--rootless-probe") == 0) {
            rootlessProbeFlag = true;
        } else if (strcmp(argv[i], "--store-init") == 0) {
            storeInitFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--store-build") == 0) {
            storeBuildFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--store-add") == 0) {
            if (i + 1 < argc) {
                storeAddPath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--store-add requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--store-sync") == 0) {
            storeSyncFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--store-sync-mode") == 0) {
            if (i + 1 < argc) {
                storeSyncModeStr = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--store-sync-mode requires file|https|sources-only");
                return 1;
            }
        } else if (strcmp(argv[i], "--store-installed") == 0) {
            storeInstalledFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--store-install") == 0) {
            if (i + 1 < argc) {
                storeInstallPkg = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--store-install requires a PACKAGE argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--store-root") == 0) {
            if (i + 1 < argc) {
                storeRootPath = argv[++i];
            } else {
                Logger::error("--store-root requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--store-publish") == 0) {
            storePublishFlag = true;
            jailbreakFlag = false;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                storePublishPath = argv[++i];
            }
        } else if (strcmp(argv[i], "--dtree-mmio") == 0) {
            if (i + 1 < argc) {
                dtreeMmioPath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--dtree-mmio requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--dtree-mmio-out") == 0) {
            if (i + 1 < argc) {
                dtreeMmioOutPath = argv[++i];
            } else {
                Logger::error("--dtree-mmio-out requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--dtree-mmio-all") == 0) {
            dtreeMmioAllFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--dtree-registers") == 0) {
            if (i + 1 < argc) {
                dtreeRegistersPath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--dtree-registers requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--dtree-registers-out") == 0) {
            if (i + 1 < argc) {
                dtreeRegistersOutPath = argv[++i];
            } else {
                Logger::error("--dtree-registers-out requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--dtree-registers-verbose") == 0) {
            dtreeRegistersVerboseFlag = true;
        } else if (strcmp(argv[i], "--hypervisor-probe") == 0) {
            if (i + 1 < argc) {
                hypervisorProbePath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--hypervisor-probe requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--hypervisor-probe-out") == 0) {
            if (i + 1 < argc) {
                hypervisorProbeOutPath = argv[++i];
            } else {
                Logger::error("--hypervisor-probe-out requires a PATH argument");
                return 1;
            }
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--integrity-probe") == 0) {
            if (i + 1 < argc) {
                integrityProbePath = argv[++i];
                jailbreakFlag = false;
            } else {
                Logger::error("--integrity-probe requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--integrity-probe-out") == 0) {
            if (i + 1 < argc) {
                integrityProbeOutPath = argv[++i];
            } else {
                Logger::error("--integrity-probe-out requires a PATH argument");
                return 1;
            }
        } else if (strcmp(argv[i], "--bypass-integrity") == 0) {
            bypassIntegrityFlag = true;
            jailbreakExecuteFlag = true;
            jailbreakFlag = false;
        } else if (strcmp(argv[i], "--probe-primitive") == 0) {
            if (i + 1 < argc) {
                probePrimitiveName = argv[++i];
            } else {
                Logger::error("--probe-primitive requires a NAME");
                return 1;
            }
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
        primitives::FutureRestoreOptions frOpts = futureRestoreCli;
        if (!apticketPath.empty() && frOpts.apticketPath.empty()) {
            frOpts.apticketPath = apticketPath;
        }
        return runTssCheck(manager, targetUDID, "", "", 0, ipswPath, apticketPath, frOpts,
                           ideviceRestoreCli)
                   ? 0
                   : 1;
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

    if (normalSshFlag) {
        ramdiskConnectCli.transport = RamdiskTransport::Ssh;
    }

    if (rootlessProbeFlag) {
        std::string summary;
        const bool ok = runRootlessProbe(ramdiskConnectCli, targetUDID, std::string(), &summary);
        if (!summary.empty()) {
            Logger::info("Rootless summary: " + summary);
        }
        return ok ? 0 : 1;
    }

    if (storeInitFlag) {
        return runStoreInit(storeRootPath) ? 0 : 1;
    }
    if (storeBuildFlag) {
        return runStoreBuild(storeRootPath) ? 0 : 1;
    }
    if (!storeAddPath.empty()) {
        return runStoreAdd(storeRootPath, storeAddPath) ? 0 : 1;
    }
    if (storePublishFlag) {
        return runStorePublish(storeRootPath, storePublishPath) ? 0 : 1;
    }
    if (storeSyncFlag) {
        if (!normalSshFlag) {
            Logger::warn("--store-sync expects --normal-ssh (or PURPLEPOIS0N_NORMAL_SSH=1)");
        }
        return runStoreSync(ramdiskConnectCli, storeRootPath, true,
                            store::parseStoreSyncMode(storeSyncModeStr))
                   ? 0
                   : 1;
    }
    if (storeInstalledFlag) {
        if (!normalSshFlag) {
            Logger::warn("--store-installed expects --normal-ssh (or PURPLEPOIS0N_NORMAL_SSH=1)");
        }
        std::vector<std::string> installed;
        if (!runStoreListInstalled(ramdiskConnectCli, &installed)) {
            return 1;
        }
        for (size_t i = 0; i < installed.size(); ++i) {
            std::cout << installed[i] << std::endl;
        }
        return 0;
    }
    if (!storeInstallPkg.empty()) {
        if (!normalSshFlag) {
            Logger::warn("--store-install expects --normal-ssh");
        }
        return runStoreInstall(ramdiskConnectCli, storeInstallPkg, true) ? 0 : 1;
    }

    if (!dtreeMmioPath.empty()) {
        return runDeviceTreeMmio(dtreeMmioPath, dtreeMmioOutPath, dtreeMmioAllFlag) ? 0 : 1;
    }

    if (!integrityProbePath.empty()) {
        return runIntegrityProbe(integrityProbePath, kernelcachePath, "", "",
                                 integrityProbeOutPath)
                   ? 0
                   : 1;
    }

    if (!hypervisorProbePath.empty()) {
        return runHypervisorProbe(hypervisorProbePath, kernelcachePath, "", hypervisorProbeOutPath)
                   ? 0
                   : 1;
    }

    if (!dtreeRegistersPath.empty()) {
        const size_t maxLog = dtreeRegistersVerboseFlag ? 0 : 64;
        return runDeviceTreeRegisterInventory(dtreeRegistersPath, dtreeRegistersOutPath, maxLog)
                   ? 0
                   : 1;
    }

    if (!probePrimitiveName.empty()) {
        primitives::PrimitiveRegistry& registry = primitives::PrimitiveRegistry::instance();
        registry.registerBuiltins();
        primitives::Primitive* primitive = registry.findByName(probePrimitiveName);
        if (primitive == nullptr) {
            Logger::error("Unknown primitive: " + probePrimitiveName);
            return 1;
        }
        primitives::ExecutionContext ctx;
        ctx.deviceState = DeviceState::Unknown;
        ctx.jailbreakGeneration = primitives::JailbreakGeneration::Gen6;
        ctx.ramdiskConnect = ramdiskConnectCli;
        if (!targetUDID.empty()) {
            ctx.udid = targetUDID;
        }
        const primitives::PrimitiveResult result = primitive->execute(ctx);
        return (result == primitives::PrimitiveResult::Success ||
                result == primitives::PrimitiveResult::ProbeOnly)
                   ? 0
                   : 1;
    }

    if (!kernelcachePath.empty()) {
        const bool allowPatch = exploitPluginsEnabled() && !patchProfilePath.empty();
        return runHostKernelPatch(kernelcachePath, patchProfilePath, patchOutPath, allowPatch)
                   ? 0
                   : 1;
    }

    auto buildCliParsed = [&]() -> CliParsedOptions {
        CliParsedOptions cli;
        cli.reportPath = reportPath;
        cli.backupPath = backupPath;
        cli.ipswPath = ipswPath;
        cli.apticketPath = apticketPath;
        cli.futureRestore = futureRestoreCli;
        cli.ideviceRestore = ideviceRestoreCli;
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
        cli.ramdiskPath = ramdiskPath;
        cli.bootLaneStr = bootLaneStr;
        cli.ramdiskFormatStr = ramdiskFormatStr;
        cli.bootModulePath = bootModulePath;
        cli.bootArgsLine = bootArgsLine;
        cli.recoveryChainFlag = recoveryChainFlag;
        cli.recoveryExecuteFlag = recoveryExecuteFlag;
        cli.pongoProbeFlag = pongoProbeFlag;
        cli.pongoBootFlag = pongoBootFlag;
        cli.pongoExecuteFlag = pongoExecuteFlag;
        cli.pongoSpawnCheckra1nFlag = pongoSpawnCheckra1nFlag;
        cli.pongoKpfPath = pongoKpfPath;
        cli.pongoRamdiskPath = pongoRamdiskPath;
        cli.pongoXargsLine = bootArgsLine.empty() ? pongoXargsLine : bootArgsLine;
        cli.postJbPipelineFlag = postJbPipelineFlag;
        cli.postJbStoreFlag = postJbStoreFlag;
        cli.postJbStoreInstallPkg = postJbStoreInstallPkg;
        cli.storeRoot = storeRootPath;
        cli.storeSyncMode = storeSyncModeStr;
        cli.medicineProbeFlag = medicineProbeFlag || medicineWithPipelineFlag;
        cli.medicineApplyFlag = medicineApplyFlag;
        cli.medicineCures = medicineCures;
        cli.medicinePlatform = medicinePlatform;
        cli.medicineCapability = medicineCapability;
        cli.medicineAppPath = medicineAppPath;
        cli.futurerestoreRestoreFlag = futurerestoreRestoreFlag;
        cli.idevicerestoreRestoreFlag = idevicerestoreRestoreFlag;
        cli.jailbreakExecuteFlag = jailbreakExecuteFlag;
        cli.bypassIntegrityFlag = bypassIntegrityFlag;
        cli.kernelcachePath = kernelcachePath;
        cli.patchProfilePath = patchProfilePath;
        cli.patchOutPath = patchOutPath;
        cli.externalJailbreakFlag = externalJailbreakFlag;
        cli.externalSkipHelperFlag = externalSkipHelperFlag;
        if (signMachoFlag || signAppFlag || signIpaFlag) {
            cli.codesignInputPath = signInputPath;
            cli.codesignOutputPath = signOutputPath;
        }
        return cli;
    };

    if (jailbreakExecuteFlag) {
        if (!understandJailbreakFlag) {
            Logger::error("jailbreak execute requires --i-understand-jailbreak");
            return 1;
        }
        bool skipPluginsForAlreadyJb = false;
        if (doctorRunFlag) {
            DeviceProfile profile;
            if (buildDeviceProfile(manager, targetUDID, &profile)) {
                const HostCapabilities host = probeHostCapabilities();
                const JailbreakPlan plan = planJailbreak(profile, host);
                skipPluginsForAlreadyJb = plan.alreadyJailbroken;
            }
        }
        if (!skipPluginsForAlreadyJb && !exploitPluginsEnabled()) {
            Logger::error("jailbreak execute requires make plugins");
            return 1;
        }
    }


    if (externalJailbreakFlag) {
        if (!externalSkipHelperFlag && !understandJailbreakFlag) {
            Logger::error("--external-jailbreak requires --i-understand-jailbreak (or --already-jailbroken)");
            return 1;
        }
        if (!externalSkipHelperFlag && !exploitPluginsEnabled()) {
            Logger::error("--external-jailbreak requires make plugins");
            return 1;
        }
        if (!normalSshFlag) {
            ramdiskConnectCli.transport = RamdiskTransport::Ssh;
        }
        Gen0Options extOpts = gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0);
        extOpts.ramdisk.connect = ramdiskConnectCli;
        if (extOpts.ramdisk.connect.udid.empty() && !targetUDID.empty()) {
            extOpts.ramdisk.connect.udid = targetUDID;
        }
        extOpts.externalJailbreak = true;
        extOpts.externalSkipHelper = externalSkipHelperFlag;
        extOpts.postJbStoreSync = postJbStoreFlag;
        extOpts.postJbStoreInstallPkg = postJbStoreInstallPkg;
        extOpts.storeRoot = storeRootPath;
        extOpts.storeSyncMode = store::parseStoreSyncMode(storeSyncModeStr);
        return runExternalJailbreak(manager, targetUDID, extOpts) ? 0 : 1;
    }

    if (dfuJailbreakFlag) {
        const DeviceState state = manager.detectDeviceState(targetUDID);
        if (state != DeviceState::DFU) {
            Logger::error("--dfu-jailbreak requires a device in DFU mode.");
            return 1;
        }
        Gen0Options dfuOpts = gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0);
        dfuOpts.reportPath = reportPath;
        dfuOpts.jailbreakExecute = true;
        dfuOpts.pongo.bootRun = true;
        dfuOpts.pongo.execute = true;
        return runDfuJailbreak(manager, dfuOpts, true) ? 0 : 1;
    }

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

    if (idevicerestoreRestoreFlag) {
        if (!understandRestoreFlag) {
            Logger::error("idevicerestore restore requires --i-understand-restore");
            return 1;
        }
        if (!exploitPluginsEnabled()) {
            Logger::error("idevicerestore restore requires make plugins");
            return 1;
        }
        Gen0Options irOpts = gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0);
        return runIdevicerestoreRestore(irOpts, true) ? 0 : 1;
    }

    codesignCli.bundleId = signBundleId;
    codesignCli.entitlementsPath = signEntPath;
    codesignCli.outputPath = signOutputPath;
    codesignCli.adHoc = adHocSign;

    if (medicineProbeFlag && !postJbPipelineFlag && !gen0Flag) {
        Gen0Options medOpts = gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0);
        medOpts.ramdisk.connect = ramdiskConnectCli;
        if (medOpts.ramdisk.connect.udid.empty() && !targetUDID.empty()) {
            medOpts.ramdisk.connect.udid = targetUDID;
        }
        return runMedicinePipeline(manager, targetUDID, medOpts, medicineApplyFlag) ? 0 : 1;
    }

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
        pipeOpts.storeRoot = storeRootPath;
        pipeOpts.storeSyncMode = store::parseStoreSyncMode(storeSyncModeStr);
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
            if (normalSshFlag || ramdiskConnectCli.transport == RamdiskTransport::Ssh) {
                ExecutionContext tcCtx;
                tcCtx.trustCachePath = trustCachePath;
                tcCtx.ramdiskConnect = ramdiskConnectCli;
                if (tcCtx.ramdiskConnect.udid.empty()) {
                    tcCtx.ramdiskConnect.udid = targetUDID;
                }
                tcCtx.ramdiskConnect.transport = RamdiskTransport::Ssh;
                return runTrustCacheAddWithContext(tcCtx, exploitPluginsEnabled()) ? 0 : 1;
            }
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
        Gen0Options dfuOpts = gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0);
        dfuOpts.reportPath = reportPath;
        if (pongoExecuteFlag && !exploitPluginsEnabled()) {
            Logger::error("-m with --pongo-execute requires make plugins");
            return 1;
        }
        return runDfuJailbreak(manager, dfuOpts, true) ? 0 : 1;
    }
    
    if (capabilitiesFlag) {
        const std::string kpfModule = resolveDefaultKpfPath();
        const std::string kpfTest = "legacy/kpf-purple/build/kpf-test-purple.macos";
        const bool moduleBuilt = isRegularFile(kpfModule);
        const bool testBuilt = isRegularFile(kpfTest);
        std::cout << "{\"plugins\":" << (exploitPluginsEnabled() ? "true" : "false")
                  << ",\"doctor\":true,\"store\":true,\"normalSsh\":true"
                  << ",\"kpf\":{\"built\":" << (moduleBuilt && testBuilt ? "true" : "false")
                  << ",\"moduleBuilt\":" << (moduleBuilt ? "true" : "false")
                  << ",\"testBuilt\":" << (testBuilt ? "true" : "false");
        if (moduleBuilt) {
            std::cout << ",\"module\":\"" << kpfModule << "\"";
        }
        if (testBuilt) {
            std::cout << ",\"test\":\"" << kpfTest << "\"";
        }
        std::cout << "}}" << std::endl;
        return 0;
    }

    if (devicePlanFlag) {
        std::string json;
        if (!runDevicePlanScan(manager, targetUDID, &json)) {
            Logger::error("No device detected for --device-plan");
            return 1;
        }
        std::cout << json << std::endl;
        return 0;
    }

    if (doctorRunFlag) {
        Gen0Options doctorOpts = gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0);
        doctorOpts.reportPath = reportPath;
        return runDoctorFlow(manager, targetUDID, doctorOpts) ? 0 : 1;
    }

    if (jailbreakFlag) {
        Gen0Options jbOpts = gen0OptionsFromCli(buildCliParsed(), Gen0CliIntent::Gen0);
        if (!performJailbreak(manager, targetUDID, reportPath, jbOpts)) {
            return 1;
        }
    }
    
    return 0;
}
