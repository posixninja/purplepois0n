/*
 * PrimitiveTypes.h
 *
 * Core enums and execution context for the primitive taxonomy framework.
 */

#ifndef PRIMITIVES_PRIMITIVE_TYPES_H_
#define PRIMITIVES_PRIMITIVE_TYPES_H_

#include "../DeviceState.h"
#include "../RamdiskTypes.h"
#include "CodesignTypes.h"
#include "TssTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PP {
namespace primitives {

class ITransport;

/** Jailbreak era detected from device state / iOS version (Phase 7 chain gating). */
enum class JailbreakGeneration {
    Unknown,
    Gen0,
    Gen1to4,
    Gen5,
    Gen6
};

/** Security domain a primitive targets. */
enum class PrimitiveCategory {
    Bootrom,
    Kernel,
    PacBypass,
    PplBypass,
    Patchfinding,
    PhysRw,
    Privilege,
    TrustCache,
    Bootstrap,
    Codesign,
    Sandbox,
    Injection
};

/** Action a primitive can perform (capability flags). */
enum class PrimitiveOperation {
    Read,
    Write,
    Overwrite,
    Patch,
    Inject,
    Execute,
    Probe
};

/** Outcome of a primitive execution attempt. */
enum class PrimitiveResult {
    Success,
    NotApplicable,
    Unsupported,
    PrerequisitesMissing,
    TransportError,
    ProbeOnly,
    PluginDisabled,
    Failed
};

/** Workflow stage labels for ChainRunner output. */
enum class ChainStage {
    Detect,
    Connect,
    Kernelcache,
    Patchfind,
    KernelExploit,
    PacBypass,
    PplBypass,
    PhysRw,
    Privilege,
    TrustCache,
    Bootstrap,
    Probe,
    Report,
    Execute
};

/** Per-run context passed to every primitive. */
struct ExecutionContext {
    DeviceState deviceState = DeviceState::Unknown;
    uint32_t cpid = 0;
    uint64_t ecid = 0;
    std::string udid;
    /** Lockdown ProductVersion when Normal mode (e.g. "16.5.1"). */
    std::string iosVersion;
    /** Lockdown ProductType when Normal mode (e.g. "iPhone14,2"). */
    std::string productType;
    /** arm64e device (PAC bypass may be required). */
    bool arm64e = false;
    bool allowMutation = false;
    ITransport* transport = nullptr;
    /** Offline backup path for BackupProbePrimitive (optional). */
    std::string backupPath;
    /** Detected jailbreak era for chain selection. */
    JailbreakGeneration jailbreakGeneration = JailbreakGeneration::Unknown;
    /** Target IPSW for TSS / futurerestore probes (optional). */
    std::string ipswPath;
    /** Saved APTicket (.shsh/.shsh2) for futurerestore path (optional). */
    std::string apticketPath;
    /** futurerestore SEP/baseband options (CLI or env). */
    FutureRestoreOptions futureRestore;
    /** IM4M manifest for personalization (from TSS or `ipsw img4 im4m extract`). */
    std::string im4mManifestPath;
    /** Unsigned component inside IPSW tree to personalize before upload. */
    std::string ipswComponentPath;
    /** Pre-signed component to upload in Recovery (iBSS/iBEC/IMG4). */
    std::string recoveryUploadPath;
    /** Component label for logs (e.g. iBSS). */
    std::string recoveryComponentLabel;
    /** ApBoardID for libtatsu BuildIdentity match (Recovery irecv). */
    uint32_t boardId = 0;
    /** Host sign input: Mach-O, .app bundle, or IPA (optional). */
    std::string codesignInputPath;
    std::string codesignOutputPath;
    CodesignOptions codesign;
    /** Normal-mode IPA install path (instproxy / ideviceinstaller). */
    std::string ipaInstallPath;
    /** Mach-O to add to on-device trust cache after jailbreak (optional). */
    std::string trustCachePath;
    /** Recovery multi-stage boot components (iBSS / iBEC / rdsk). */
    struct RecoveryChainComponent {
        std::string fourcc;
        std::string ipswComponentPath;
        std::string uploadPath;
    };
    std::vector<RecoveryChainComponent> recoveryChain;
    RamdiskOptions ramdiskBuild;
    std::string ramdiskOverlayPath;
    std::vector<RamdiskStageEntry> ramdiskStagedFiles;
    std::string ramdiskWorkDir;
    std::string ramdiskIdent;
    bool recoveryChainRun = false;
    /** Live comm to booted ramdisk (usbmux + iproxy). */
    RamdiskConnectOptions ramdiskConnect;
    std::string ramdiskExecCommand;
    std::string ramdiskUploadLocal;
    std::string ramdiskUploadRemote;
    std::string ramdiskDownloadRemote;
    std::string ramdiskDownloadLocal;
    std::string ramdiskListPath;
    /** PongoOS (checkra1n DFU path) — USB 05ac:4141, separate from Recovery. */
    bool pongoProbeRun = false;
    bool pongoBootRun = false;
    bool pongoSpawnCheckra1n = false;
    std::string pongoKpfPath;
    std::string pongoRamdiskDmgPath;
    std::string pongoXargsLine;
};

/** Returns true when built with PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS. */
bool exploitPluginsEnabled();

/** Human-readable names for logging and reports. */
const char* categoryToString(PrimitiveCategory category);
const char* operationToString(PrimitiveOperation operation);
const char* resultToString(PrimitiveResult result);
const char* stageToString(ChainStage stage);

/** Test whether @p ops contains @p op. */
bool supportsOperation(const std::vector<PrimitiveOperation>& ops, PrimitiveOperation op);

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_PRIMITIVE_TYPES_H_ */
