/*
 * RamdiskShellPrimitive.cpp
 */

#include "primitives/RamdiskShellPrimitive.h"
#include "RamdiskClient.h"
#include "Logger.h"

#include <cstdlib>
#include <iostream>

namespace PP {
namespace primitives {

const char* RamdiskShellPrimitive::name() const { return "ramdisk-shell"; }

PrimitiveCategory RamdiskShellPrimitive::category() const {
    return PrimitiveCategory::Injection;
}

std::vector<PrimitiveOperation> RamdiskShellPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> RamdiskShellPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Normal, DeviceState::Unknown};
}

bool RamdiskShellPrimitive::canRun(const ExecutionContext& context) const {
    if (!context.ramdiskExecCommand.empty() || !context.ramdiskUploadLocal.empty() ||
        !context.ramdiskDownloadRemote.empty() || !context.ramdiskListPath.empty()) {
        return true;
    }
    const char* env = std::getenv("PURPLEPOIS0N_RAMDISK_TCP_PORT");
    if (env != nullptr && env[0] != '\0') {
        return true;
    }
    env = std::getenv("PURPLEPOIS0N_RAMDISK_SSH_PORT");
    if (env != nullptr && env[0] != '\0') {
        return true;
    }
    env = std::getenv("PURPLEPOIS0N_RAMDISK_TRANSPORT");
    return env != nullptr && env[0] != '\0';
}

PrimitiveResult RamdiskShellPrimitive::execute(ExecutionContext& context) {
    Logger::info("  [Ramdisk] live comm (" + ramdiskTransportName(context.ramdiskConnect.transport) +
                 " — custom overlay agent; stock IPSW rdsk uses restore protocol)");

    RamdiskConnectOptions opts = context.ramdiskConnect;
    if (opts.host.empty()) {
        opts.host = "127.0.0.1";
    }
    if (opts.transport == RamdiskTransport::TcpLine && opts.tcpPort == 0) {
        opts.tcpPort = 4444;
    }
    if (opts.transport == RamdiskTransport::Ssh && opts.sshPort == 0) {
        opts.sshPort = 2222;
    }
    if (opts.udid.empty() && !context.udid.empty()) {
        opts.udid = context.udid;
    }

    RamdiskClient client(opts);
    std::string probeMsg;
    if (!client.probe(&probeMsg)) {
        Logger::warn("  [Ramdisk] probe failed: " + probeMsg);
        Logger::info("  [Ramdisk] stage an arm64 agent with --ramdisk-add and start it from overlay init");
        Logger::info("  [Ramdisk] forward with iproxy -u UDID 4444:4444 (or --ramdisk-transport ssh)");
        return PrimitiveResult::PrerequisitesMissing;
    }
    Logger::info("  [Ramdisk] " + probeMsg);

    if (!context.allowMutation) {
        if (!context.ramdiskExecCommand.empty()) {
            Logger::info("  [Ramdisk] would exec: " + context.ramdiskExecCommand);
        }
        if (!context.ramdiskUploadLocal.empty()) {
            Logger::info("  [Ramdisk] would upload " + context.ramdiskUploadLocal + " → " +
                         context.ramdiskUploadRemote);
        }
        if (!context.ramdiskDownloadRemote.empty()) {
            Logger::info("  [Ramdisk] would download " + context.ramdiskDownloadRemote + " → " +
                         context.ramdiskDownloadLocal);
        }
        if (!context.ramdiskListPath.empty()) {
            Logger::info("  [Ramdisk] would list: " + context.ramdiskListPath);
        }
        return PrimitiveResult::Success;
    }

    if (!context.ramdiskUploadLocal.empty()) {
        if (!client.uploadFile(context.ramdiskUploadLocal, context.ramdiskUploadRemote)) {
            return PrimitiveResult::Failed;
        }
    }
    if (!context.ramdiskDownloadRemote.empty()) {
        if (!client.downloadFile(context.ramdiskDownloadRemote, context.ramdiskDownloadLocal)) {
            return PrimitiveResult::Failed;
        }
    }
    if (!context.ramdiskListPath.empty()) {
        const RamdiskCommandResult listed = client.listDirectory(context.ramdiskListPath);
        if (listed.exitCode != 0) {
            Logger::error("  [Ramdisk] ls failed: " + listed.stderrText);
            return PrimitiveResult::Failed;
        }
        std::cout << listed.stdoutText;
    }
    if (!context.ramdiskExecCommand.empty()) {
        const RamdiskCommandResult ran = client.exec(context.ramdiskExecCommand);
        if (!ran.stdoutText.empty()) {
            std::cout << ran.stdoutText;
        }
        if (ran.exitCode != 0) {
            Logger::error("  [Ramdisk] command failed: " + ran.stderrText);
            return PrimitiveResult::Failed;
        }
    }

    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
