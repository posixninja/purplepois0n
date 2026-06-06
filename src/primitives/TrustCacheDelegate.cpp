/*
 * TrustCacheDelegate.cpp
 */

#include "primitives/TrustCacheDelegate.h"
#include "primitives/Gen6Types.h"
#include "RamdiskClient.h"
#include "RamdiskTypes.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace PP {
namespace primitives {

namespace {

bool trustCacheMutationAllowed(const ExecutionContext& context) {
    JailbreakGeneration era = context.jailbreakGeneration;
    if (era == JailbreakGeneration::Unknown) {
        era = detectJailbreakGeneration(context);
    }
    if (era == JailbreakGeneration::Gen5 || era == JailbreakGeneration::Gen6) {
        return true;
    }
    return iosVersionInRange(context.iosVersion, "15.0", "99.0");
}

bool ramdiskTransportConfigured(const ExecutionContext& context) {
    const RamdiskConnectOptions& connect = context.ramdiskConnect;
    if (!connect.udid.empty()) {
        return true;
    }
    if (connect.transport == RamdiskTransport::Ssh) {
        return true;
    }
    return false;
}

std::string basenamePath(const std::string& path) {
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        return path;
    }
    return path.substr(slash + 1);
}

PrimitiveResult addViaRamdisk(const ExecutionContext& context, const std::string& binary,
                              bool allowMutation) {
    Logger::info("  [TrustCache] ramdisk transport — upload + jbctl on device");

    if (!allowMutation) {
        Logger::info("  [TrustCache] would upload " + binary + " and run jbctl trustcache add");
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [TrustCache] trustcache add requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    RamdiskClient client(context.ramdiskConnect);
    std::string message;
    if (!client.probe(&message)) {
        Logger::error("  [TrustCache] ramdisk probe failed: " + message);
        return PrimitiveResult::TransportError;
    }

    const std::string remotePath = std::string("/tmp/") + basenamePath(binary);
    if (!client.uploadFile(binary, remotePath)) {
        Logger::error("  [TrustCache] ramdisk upload failed");
        return PrimitiveResult::Failed;
    }

    const std::string cmd = "jbctl trustcache add " + remotePath;
    const RamdiskCommandResult result = client.exec(cmd);
    if (result.exitCode != 0) {
        Logger::error("  [TrustCache] on-device jbctl failed: " + result.stderrText);
        return PrimitiveResult::Failed;
    }

    Logger::info("  [TrustCache] on-device trustcache add complete");
    return PrimitiveResult::Success;
}

} /* anonymous */

std::string TrustCacheDelegate::findJbctl() {
    const char* env = std::getenv("PURPLEPOIS0N_JBCTL");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }
    return ToolRunner::findExecutable("jbctl");
}

std::string TrustCacheDelegate::findTrustCacheTool() {
    const char* env = std::getenv("PURPLEPOIS0N_TRUSTCACHE_TOOL");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }
    return std::string();
}

std::string TrustCacheDelegate::resolveBinaryPath(const ExecutionContext& context) {
    if (!context.trustCachePath.empty()) {
        return context.trustCachePath;
    }
    const char* env = std::getenv("PURPLEPOIS0N_TRUSTCACHE_PATH");
    return (env != nullptr) ? std::string(env) : std::string();
}

void TrustCacheDelegate::logToolOverview() {
    Logger::info("  [TrustCache] on-device pseudo-signing (post-kernel jailbreak)");
    Logger::info("  [TrustCache] Fugu15/Dopamine: jbctl trustcache / CoreTrust — not in-tree");
    if (!findJbctl().empty()) {
        Logger::info("  [TrustCache] jbctl: " + findJbctl());
    } else {
        Logger::warn("  [TrustCache] jbctl not found (PURPLEPOIS0N_JBCTL)");
    }
    const std::string tool = findTrustCacheTool();
    if (!tool.empty()) {
        Logger::info("  [TrustCache] tool: " + tool);
    }
}

PrimitiveResult TrustCacheDelegate::addToTrustCache(const ExecutionContext& context,
                                                    bool allowMutation) {
    logToolOverview();

    const std::string binary = resolveBinaryPath(context);
    if (binary.empty()) {
        Logger::info("  [TrustCache] set PURPLEPOIS0N_TRUSTCACHE_PATH or --trustcache-add PATH");
        return PrimitiveResult::Success;
    }

    if (ramdiskTransportConfigured(context)) {
        return addViaRamdisk(context, binary, allowMutation);
    }

    const std::string jbctl = findJbctl();
    const std::string genericTool = findTrustCacheTool();

    std::vector<std::string> argv;
    if (!jbctl.empty()) {
        argv.push_back(jbctl);
        argv.push_back("trustcache");
        argv.push_back("add");
        argv.push_back(binary);
    } else if (!genericTool.empty()) {
        argv.push_back(genericTool);
        argv.push_back(binary);
    } else {
        Logger::warn("  [TrustCache] no jbctl or PURPLEPOIS0N_TRUSTCACHE_TOOL configured");
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (!allowMutation) {
        std::ostringstream line;
        for (size_t i = 0; i < argv.size(); ++i) {
            if (i > 0) {
                line << ' ';
            }
            line << argv[i];
        }
        Logger::info("  [TrustCache] would run (probe only): " + line.str());
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [TrustCache] trustcache add requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    if (!trustCacheMutationAllowed(context)) {
        Logger::warn("  [TrustCache] execute gated — requires Gen5+ era or iOS 15+");
        return PrimitiveResult::NotApplicable;
    }

    Logger::info("  [TrustCache] adding to trust cache (host): " + binary);
    const CommandResult result = ToolRunner::run(argv);
    return (result.exitCode == 0) ? PrimitiveResult::Success : PrimitiveResult::Failed;
}

} /* namespace primitives */
} /* namespace PP */
