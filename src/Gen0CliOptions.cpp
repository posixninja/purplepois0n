/*
 * Gen0CliOptions.cpp
 */

#include "Gen0CliOptions.h"

namespace PP {

Gen0Options gen0OptionsFromCli(const CliParsedOptions& cli, Gen0CliIntent intent) {
    Gen0Options options;
    options.reportPath = cli.reportPath;
    options.backupPath = cli.backupPath;
    options.ipswPath = cli.ipswPath;
    options.apticketPath = cli.apticketPath;
    options.futureRestore = cli.futureRestore;
    if (!cli.apticketPath.empty()) {
        options.futureRestore.apticketPath = cli.apticketPath;
    }
    options.im4mManifestPath = cli.im4mManifestPath;
    options.ipswComponentPath = cli.ipswComponentPath;
    options.recovery.uploadPath = cli.recoveryUploadPath;
    options.recovery.componentLabel = cli.recoveryComponentLabel;
    options.codesignInputPath = cli.codesignInputPath;
    options.codesignOutputPath = cli.codesignOutputPath;
    options.codesign = cli.codesign;
    options.ipaInstallPath = cli.ipaInstallPath;
    options.trustCachePath = cli.trustCachePath;
    options.ramdisk.buildRamdiskPath = cli.buildRamdiskPath;
    options.ramdisk.fromIpswOutput = cli.ramdiskFromIpswOutput;
    options.ramdisk.overlayPath = cli.ramdiskOverlayPath;
    options.ramdisk.stagedFiles = cli.ramdiskStagedFiles;
    options.ramdisk.workDir = cli.ramdiskWorkDir;
    options.ramdisk.ident = cli.ramdiskIdent;
    options.ramdisk.build = cli.ramdiskBuild;
    options.ramdisk.connect = cli.ramdiskConnect;
    options.ramdisk.execCommand = cli.ramdiskExecCommand;
    options.ramdisk.uploadLocal = cli.ramdiskUploadLocal;
    options.ramdisk.uploadRemote = cli.ramdiskUploadRemote;
    options.ramdisk.downloadRemote = cli.ramdiskDownloadRemote;
    options.ramdisk.downloadLocal = cli.ramdiskDownloadLocal;
    options.ramdisk.listPath = cli.ramdiskListPath;
    options.pongo.probeRun = cli.pongoProbeFlag;
    options.pongo.bootRun = cli.pongoBootFlag;
    options.pongo.execute = cli.pongoExecuteFlag;
    options.pongo.spawnCheckra1n = cli.pongoSpawnCheckra1nFlag;
    options.pongo.kpfPath = cli.pongoKpfPath;
    options.pongo.ramdiskDmgPath =
        cli.pongoRamdiskPath.empty() ? cli.buildRamdiskPath : cli.pongoRamdiskPath;
    options.pongo.xargsLine = cli.pongoXargsLine;
    options.postJbPipeline = cli.postJbPipelineFlag;
    options.postJbStoreSync = cli.postJbStoreFlag;
    options.postJbStoreInstallPkg = cli.postJbStoreInstallPkg;
    options.storeRoot = cli.storeRoot;
    options.medicineRun = cli.medicineProbeFlag || cli.medicineApplyFlag;
    options.medicineApply = cli.medicineApplyFlag;
    options.medicineCures = cli.medicineCures;
    options.medicinePlatform = cli.medicinePlatform;
    options.medicineCapability = cli.medicineCapability;
    options.medicineAppPath = cli.medicineAppPath;
    options.futurerestoreRestore = cli.futurerestoreRestoreFlag;
    options.jailbreakExecute = cli.jailbreakExecuteFlag;
    options.bypassIntegrity = cli.bypassIntegrityFlag || cli.jailbreakExecuteFlag;
    if (cli.jailbreakExecuteFlag && !cli.pongoProbeFlag && !cli.pongoBootFlag) {
        options.pongo.bootRun = true;
        options.pongo.execute = true;
    }
    options.kernelcachePath = cli.kernelcachePath;
    options.patchProfilePath = cli.patchProfilePath;
    options.patchOutPath = cli.patchOutPath;

    switch (intent) {
        case Gen0CliIntent::RecoveryChain:
            options.recovery.chainRun = cli.recoveryChainFlag || cli.recoveryExecuteFlag;
            options.recovery.execute = cli.recoveryExecuteFlag;
            break;
        case Gen0CliIntent::PongoBoot:
            options.pongo.bootRun = true;
            break;
        case Gen0CliIntent::Gen0:
        default:
            options.recovery.chainRun = cli.recoveryChainFlag;
            options.recovery.execute = cli.recoveryExecuteFlag;
            break;
    }
    return options;
}

} /* namespace PP */
