/*
 * Gen0Workflow.h
 *
 * Generation 0 (greenpois0n / absinthe) scaffolding: honest status messages
 * and educational tooling without exploit or backup weaponization.
 */

#ifndef GEN0_WORKFLOW_H_
#define GEN0_WORKFLOW_H_

#include <cstdint>
#include <string>
#include <vector>
#include "MachOParser.h"
#include "primitives/CodesignTypes.h"
#include "primitives/TssTypes.h"
#include "primitives/PrimitiveTypes.h"
#include "RamdiskTypes.h"
#include "store/DpkgStoreSync.h"

namespace PP {

class DeviceManager;

struct Gen0RecoveryOptions {
    bool chainRun = false;
    bool execute = false;
    std::string uploadPath;
    std::string componentLabel;
    std::vector<primitives::ExecutionContext::RecoveryChainComponent> chain;
};

struct Gen0RamdiskOptions {
    std::string buildRamdiskPath;
    std::string fromIpswOutput;
    std::string overlayPath;
    std::string workDir;
    std::string ident;
    RamdiskOptions build;
    std::vector<RamdiskStageEntry> stagedFiles;
    RamdiskConnectOptions connect;
    std::string execCommand;
    std::string uploadLocal;
    std::string uploadRemote;
    std::string downloadRemote;
    std::string downloadLocal;
    std::string listPath;
    /** Generic delivery artifact (--ramdisk); aliases legacy --pongo-ramdisk. */
    std::string artifactPath;
    BootDeliveryLane deliveryLane = BootDeliveryLane::Auto;
    RamdiskArtifactFormat artifactFormat = RamdiskArtifactFormat::Auto;
    std::string bootModulePath;
    std::string bootArgsLine;
    bool deliveryRun = false;
    bool deliveryProbe = false;
};

struct Gen0PongoOptions {
    bool probeRun = false;
    bool bootRun = false;
    bool execute = false;
    bool spawnCheckra1n = false;
    std::string kpfPath;
    std::string ramdiskDmgPath;
    std::string xargsLine;
};

/** Options for Gen0 scaffold runs. */
struct Gen0Options {
    std::string reportPath;
    std::string backupPath;
    std::string ipswPath;
    std::string apticketPath;
    primitives::FutureRestoreOptions futureRestore;
    std::string im4mManifestPath;
    std::string ipswComponentPath;
    std::string fetchShshPath;
    std::string codesignInputPath;
    std::string codesignOutputPath;
    primitives::CodesignOptions codesign;
    std::string ipaInstallPath;
    std::string trustCachePath;
    Gen0RecoveryOptions recovery;
    Gen0RamdiskOptions ramdisk;
    Gen0PongoOptions pongo;
    /** Run sign-ipa → install-ipa → trustcache-add when mutation enabled. */
    bool postJbPipeline = false;
    /** Post-install medicine cures after jailbreak (afc2, capable, loader hints). */
    bool medicineRun = false;
    bool medicineApply = false;
    std::string medicineCures;
    std::string medicinePlatform;
    std::string medicineCapability;
    std::string medicineAppPath;
    bool futurerestoreRestore = false;
    /** Run era execute chain (requires make plugins + --i-understand-jailbreak). */
    bool jailbreakExecute = false;
    /** Auto-run PAC + data-integrity bypass (badRecovery delegate) after kernel exploit. */
    bool bypassIntegrity = false;
    std::string kernelcachePath;
    std::string patchProfilePath;
    std::string patchOutPath;
    /** Sync purplepois0n-store to device during post-jb pipeline. */
    bool postJbStoreSync = false;
    std::string storeRoot;
    std::string postJbStoreInstallPkg;
    store::StoreSyncMode storeSyncMode = store::StoreSyncMode::File;
    /** Delegate jailbreak to palera1n/checkra1n wrapper (--external-jailbreak). */
    bool externalJailbreak = false;
    /** Skip external helper; probe /var/jb and run post-jb store only. */
    bool externalSkipHelper = false;
};

bool runGen0Jailbreak(DeviceManager& manager,
                      const std::string& targetUDID = "",
                      const Gen0Options& options = Gen0Options());

bool analyzeBackup(const std::string& backupPath);

bool analyzeBinary(const std::string& binaryPath,
                   MachOArchPreference archPreference = MachOArchPreference::Default,
                   const std::string& payloadJsonPath = "");

bool analyzeDyldCache(const std::string& cachePath,
                      const std::string& payloadJsonPath = "");

bool analyzeCrashLog(const std::string& crashPath);

bool runTssCheck(DeviceManager& manager,
                 const std::string& targetUDID,
                 const std::string& productType,
                 const std::string& iosVersion,
                 uint64_t ecid,
                 const std::string& ipswPath = "",
                 const std::string& apticketPath = "");

bool fetchLiveShsh(DeviceManager& manager,
                   const std::string& targetUDID,
                   const std::string& ipswPath,
                   const std::string& outputPath);

bool runHostCodesign(const std::string& inputPath,
                     const primitives::CodesignOptions& options,
                     bool allowMutation);

bool runHostSignIpa(const std::string& ipaPath,
                    const std::string& outputIpaPath,
                    const primitives::CodesignOptions& options,
                    bool allowMutation);

bool runSideloadInstall(DeviceManager& manager,
                         const std::string& targetUDID,
                         const std::string& ipaPath,
                         bool allowMutation);

bool runTrustCacheAdd(const std::string& binaryPath, bool allowMutation);

/** Host jbctl or ramdisk SSH trust-cache add (uses @p context ramdiskConnect when set). */
bool runTrustCacheAddWithContext(const primitives::ExecutionContext& context, bool allowMutation);

/** Sign IPA → install → optional trustcache (requires make plugins + Normal mode). */
bool runPostJbPipeline(DeviceManager& manager,
                       const std::string& targetUDID,
                       const Gen0Options& options);

/** Probe or apply Chronic-Dev medicine-style post-install cures. */
bool runMedicinePipeline(DeviceManager& manager,
                         const std::string& targetUDID,
                         const Gen0Options& options,
                         bool apply);

/** Destructive futurerestore restore (explicit CLI only). */
bool runFuturerestoreRestore(const Gen0Options& options, bool allowMutation);

/** Offline host kernelcache patchfind / apply (no device required). */
bool runHostKernelPatch(const std::string& kernelcachePath,
                        const std::string& patchProfilePath,
                        const std::string& patchOutPath,
                        bool allowMutation);

bool runBuildRamdisk(const RamdiskOptions& options, const std::string& overlayDir,
                     const std::vector<RamdiskStageEntry>& stagedFiles,
                     const std::string& outputPath);

bool runRamdiskFromIpsw(const RamdiskOptions& options, const std::string& ipswPath,
                        const std::string& ident, const std::string& overlayDir,
                        const std::vector<RamdiskStageEntry>& stagedFiles,
                        const std::string& workDir, const std::string& outputPath);

bool populateDefaultRecoveryChain(const std::string& ipswPath, const std::string& workDir,
    std::vector<primitives::ExecutionContext::RecoveryChainComponent>* chain);

bool runRamdiskProbe(const RamdiskConnectOptions& options, std::string* message);

/** Probe /var/jb bootstrap layout over SSH (jailbroken Normal device). */
bool runRootlessProbe(const RamdiskConnectOptions& connect, const std::string& udid,
                      const std::string& iosVersion, std::string* summary);

bool runStoreInit(const std::string& storeRoot);
bool runStoreBuild(const std::string& storeRoot);
bool runStoreAdd(const std::string& storeRoot, const std::string& debPath);
bool runStoreSync(const RamdiskConnectOptions& connect, const std::string& storeRoot,
                  bool allowMutation,
                  store::StoreSyncMode mode = store::StoreSyncMode::File);
bool runStoreInstall(const RamdiskConnectOptions& connect, const std::string& packageName,
                     bool allowMutation);
bool runStoreListInstalled(const RamdiskConnectOptions& connect, std::vector<std::string>* packages);

bool runStorePublish(const std::string& storeRoot, const std::string& publishRoot);

bool runDeviceTreeMmio(const std::string& inputPath, const std::string& outJsonPath,
                       bool includeAllRegions);

bool runDeviceTreeRegisterInventory(const std::string& inputPath, const std::string& outJsonPath,
                                    size_t maxLogEntries);

/** Offline SPTM/hypervisor page-monitor profile from DT + optional kernelcache. */
bool runHypervisorProbe(const std::string& inputPath, const std::string& kernelcachePath,
                        const std::string& iosVersion, const std::string& outJsonPath);

bool runIntegrityProbe(const std::string& inputPath, const std::string& kernelcachePath,
                       const std::string& productType, const std::string& iosVersion,
                       const std::string& outJsonPath);

bool runRamdiskExec(const RamdiskConnectOptions& options, const std::string& command);

bool runRamdiskPush(const RamdiskConnectOptions& options, const std::string& localPath,
                    const std::string& remotePath);

bool runRamdiskPull(const RamdiskConnectOptions& options, const std::string& remotePath,
                    const std::string& localPath);

bool runRamdiskList(const RamdiskConnectOptions& options, const std::string& remotePath);

/** PongoOS USB probe/boot (see src/pongo/PongoWorkflow.cpp). */
bool runPongoProbe(bool spawnCheckra1n, bool allowMutation, std::string* message);
bool runPongoBoot(const Gen0Options& options, bool allowMutation);

/**
 * DFU orchestration: probe chain → bootrom pwn (gaster/checkm8) → optional Pongo KPF+ramdisk.
 * @p executeBootrom  When false, probe-only (same as default -j in DFU).
 */
bool runDfuJailbreak(DeviceManager& manager, const Gen0Options& options, bool executeBootrom);

bool runExternalJailbreak(DeviceManager& manager,
                          const std::string& targetUDID,
                          const Gen0Options& options);

} /* namespace PP */

#endif /* GEN0_WORKFLOW_H_ */
