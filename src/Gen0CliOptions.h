/*
 * Gen0CliOptions.h
 *
 * Map parsed CLI state into Gen0Options.
 */

#ifndef GEN0_CLI_OPTIONS_H_
#define GEN0_CLI_OPTIONS_H_

#include "Gen0Workflow.h"
#include "primitives/CodesignTypes.h"
#include "primitives/TssTypes.h"
#include "RamdiskTypes.h"

#include <string>
#include <vector>

namespace PP {

enum class Gen0CliIntent { Gen0, RecoveryChain, PongoBoot };

struct CliParsedOptions {
    std::string reportPath;
    std::string backupPath;
    std::string ipswPath;
    std::string apticketPath;
    primitives::FutureRestoreOptions futureRestore;
    std::string im4mManifestPath;
    std::string ipswComponentPath;
    std::string recoveryUploadPath;
    std::string recoveryComponentLabel;
    std::string codesignInputPath;
    std::string codesignOutputPath;
    primitives::CodesignOptions codesign;
    std::string ipaInstallPath;
    std::string trustCachePath;
    std::string buildRamdiskPath;
    std::string ramdiskFromIpswOutput;
    std::string ramdiskOverlayPath;
    std::string ramdiskWorkDir;
    std::string ramdiskIdent;
    RamdiskOptions ramdiskBuild;
    std::vector<RamdiskStageEntry> ramdiskStagedFiles;
    RamdiskConnectOptions ramdiskConnect;
    std::string ramdiskExecCommand;
    std::string ramdiskUploadLocal;
    std::string ramdiskUploadRemote;
    std::string ramdiskDownloadRemote;
    std::string ramdiskDownloadLocal;
    std::string ramdiskListPath;
    bool recoveryChainFlag = false;
    bool recoveryExecuteFlag = false;
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
};

Gen0Options gen0OptionsFromCli(const CliParsedOptions& cli, Gen0CliIntent intent);

} /* namespace PP */

#endif /* GEN0_CLI_OPTIONS_H_ */
