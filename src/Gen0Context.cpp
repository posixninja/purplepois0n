/*
 * Gen0Context.cpp
 */

#include "Gen0Context.h"
#include "RamdiskDelivery.h"
#include "primitives/TssTypes.h"

namespace PP {

primitives::ExecutionContext buildExecutionContext(DeviceState state,
                                                   const Gen0Options& options,
                                                   const std::string& udid,
                                                   uint64_t ecid,
                                                   bool allowMutation) {
    primitives::ExecutionContext ctx;
    ctx.deviceState = state;
    ctx.udid = udid;
    ctx.ecid = ecid;
    ctx.allowMutation = allowMutation;
    ctx.ipswPath = options.ipswPath;
    ctx.apticketPath = options.apticketPath;
    ctx.futureRestore = options.futureRestore;
    ctx.ideviceRestore = options.ideviceRestore;
    ctx.im4mManifestPath = options.im4mManifestPath;
    ctx.ipswComponentPath = options.ipswComponentPath;
    ctx.recoveryUploadPath = options.recovery.uploadPath;
    ctx.recoveryComponentLabel = options.recovery.componentLabel;
    if (ctx.futureRestore.apticketPath.empty() && !ctx.apticketPath.empty()) {
        ctx.futureRestore.apticketPath = ctx.apticketPath;
    }
    if (ctx.futureRestore.apticketPath.empty()) {
        ctx.futureRestore = primitives::futureRestoreOptionsFromEnv();
        if (!ctx.apticketPath.empty()) {
            ctx.futureRestore.apticketPath = ctx.apticketPath;
        }
    }
    ctx.codesignInputPath = options.codesignInputPath;
    ctx.codesignOutputPath = options.codesignOutputPath;
    ctx.codesign = options.codesign;
    ctx.ipaInstallPath = options.ipaInstallPath;
    ctx.trustCachePath = options.trustCachePath;
    ctx.ramdiskBuild = options.ramdisk.build;
    ctx.ramdiskOverlayPath = options.ramdisk.overlayPath;
    ctx.ramdiskStagedFiles = options.ramdisk.stagedFiles;
    ctx.ramdiskConnect = options.ramdisk.connect;
    ctx.ramdiskExecCommand = options.ramdisk.execCommand;
    ctx.ramdiskUploadLocal = options.ramdisk.uploadLocal;
    ctx.ramdiskUploadRemote = options.ramdisk.uploadRemote;
    ctx.ramdiskDownloadRemote = options.ramdisk.downloadRemote;
    ctx.ramdiskDownloadLocal = options.ramdisk.downloadLocal;
    ctx.ramdiskListPath = options.ramdisk.listPath;
    ctx.ramdiskWorkDir = options.ramdisk.workDir;
    ctx.ramdiskIdent =
        options.ramdisk.ident.empty() ? std::string("Erase") : options.ramdisk.ident;
    ctx.recoveryChain = options.recovery.chain;
    ctx.recoveryChainRun = options.recovery.chainRun;
    ctx.bootDeliveryRun = options.ramdisk.deliveryRun || options.pongo.bootRun;
    ctx.bootDeliveryProbe = options.ramdisk.deliveryProbe || options.pongo.probeRun;
    ctx.pongoProbeRun = options.pongo.probeRun || ctx.bootDeliveryProbe;
    ctx.pongoBootRun = options.pongo.bootRun || ctx.bootDeliveryRun;
    ctx.pongoSpawnCheckra1n = options.pongo.spawnCheckra1n;
    ctx.pongoKpfPath = options.pongo.kpfPath;
    if (ctx.pongoKpfPath.empty()) {
        ctx.pongoKpfPath = resolveDefaultBootModulePath();
    }
    ctx.ramdiskArtifactPath = options.ramdisk.artifactPath;
    if (ctx.ramdiskArtifactPath.empty()) {
        ctx.ramdiskArtifactPath = options.pongo.ramdiskDmgPath;
    }
    if (ctx.ramdiskArtifactPath.empty() && !options.ramdisk.buildRamdiskPath.empty()) {
        ctx.ramdiskArtifactPath = options.ramdisk.buildRamdiskPath;
    }
    ctx.pongoRamdiskDmgPath = ctx.ramdiskArtifactPath;
    ctx.pongoXargsLine = options.pongo.xargsLine;
    ctx.ramdiskArtifactFormat = options.ramdisk.artifactFormat;
    ctx.bootDeliveryLane = options.ramdisk.deliveryLane;
    ctx.bootModulePath = options.ramdisk.bootModulePath;
    if (ctx.bootModulePath.empty()) {
        ctx.bootModulePath = ctx.pongoKpfPath;
    } else {
        ctx.pongoKpfPath = ctx.bootModulePath;
    }
    ctx.bootArgsLine = options.ramdisk.bootArgsLine;
    if (!ctx.bootArgsLine.empty()) {
        ctx.pongoXargsLine = ctx.bootArgsLine;
    } else if (!ctx.pongoXargsLine.empty()) {
        ctx.bootArgsLine = ctx.pongoXargsLine;
    }
    ctx.bypassIntegrityRun = options.bypassIntegrity;
    ctx.kernelcachePath = options.kernelcachePath;
    ctx.patchProfilePath = options.patchProfilePath;
    ctx.patchOutPath = options.patchOutPath;
    return ctx;
}

} /* namespace PP */
