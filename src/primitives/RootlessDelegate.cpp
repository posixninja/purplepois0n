/*
 * RootlessDelegate.cpp
 */

#include "primitives/RootlessDelegate.h"
#include "primitives/Gen6Types.h"
#include "primitives/JbHelperDelegate.h"
#include "RamdiskClient.h"
#include "EnvUtil.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace PP {
namespace primitives {

namespace {

std::vector<std::string> splitArgs(const std::string& text) {
    std::vector<std::string> out;
    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == ' ' || c == '\t') {
            if (!current.empty()) {
                out.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

bool helperExecutable(const char* envName, std::string* outPath) {
    const char* path = std::getenv(envName);
    if (path == nullptr || path[0] == '\0' || access(path, X_OK) != 0) {
        return false;
    }
    if (outPath != nullptr) {
        *outPath = path;
    }
    return true;
}

PrimitiveResult spawnHelper(const std::string& helperPath, const char* argsEnv,
                            const ExecutionContext& context, bool allowMutation,
                            const char* logTag) {
    if (!allowMutation) {
        Logger::info(std::string("  [Rootless]   ") + logTag + " configured: " + helperPath);
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn(std::string("  [Rootless]   ") + logTag + " spawn requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    std::vector<std::string> argv;
    argv.push_back(helperPath);
    argv.push_back("--rootless");
    argv.push_back("--jbroot");
    argv.push_back(RootlessLayout::resolveJbroot());

    const char* extraArgs = std::getenv(argsEnv);
    if (extraArgs != nullptr && extraArgs[0] != '\0') {
        const std::vector<std::string> parts = splitArgs(extraArgs);
        for (size_t i = 0; i < parts.size(); ++i) {
            argv.push_back(parts[i]);
        }
    }
    if (!context.udid.empty()) {
        argv.push_back("--udid");
        argv.push_back(context.udid);
    }

    Logger::info(std::string("  [Rootless]   spawning ") + logTag + ": " + helperPath);
    const CommandResult result = ToolRunner::run(argv);
    if (result.exitCode == 0) {
        Logger::info(std::string("  [Rootless]   ") + logTag + " exited 0");
        return PrimitiveResult::Success;
    }

    Logger::error(std::string("  [Rootless]   ") + logTag + " exit " +
                  std::to_string(result.exitCode));
    if (!result.stderrText.empty()) {
        Logger::warn(std::string("  [Rootless]   stderr: ") + result.stderrText);
    }
    return PrimitiveResult::Failed;
}

void appendMarkerFromLine(const std::string& line, RootlessProbeResult* result) {
    if (line == "JBROOT_OK") {
        result->jbrootExists = true;
    } else if (line == "BINDMOUNT") {
        result->bindMountsVisible = true;
    } else if (line == "ROOTFUL_RW") {
        result->rootfulWritableSystem = true;
    } else if (line == "FLAVOR_DOPAMINE") {
        result->bootstrapFlavor = "dopamine";
        result->foundMarkers.push_back("dopamine");
    } else if (line == "FLAVOR_PALERA1N") {
        result->bootstrapFlavor = "palera1n";
        result->foundMarkers.push_back("palera1n");
    } else if (line == "FLAVOR_PROCURSUS") {
        if (result->bootstrapFlavor.empty()) {
            result->bootstrapFlavor = "procursus";
        }
        result->foundMarkers.push_back("procursus");
    } else if (line == "MARKER_JBCTL") {
        result->foundMarkers.push_back("jbctl");
    } else if (line == "MARKER_DPKG") {
        result->foundMarkers.push_back("dpkg");
    } else if (line == "MARKER_BASH") {
        result->foundMarkers.push_back("bash");
    } else if (line == "MARKER_LAUNCHD") {
        result->foundMarkers.push_back("launchdaemons");
    } else if (line.rfind("MISSING:", 0) == 0) {
        result->missingRequired.push_back(line.substr(8));
    }
}

std::string buildRemoteProbeScript(const std::string& jbroot) {
    std::ostringstream script;
    script << "JB='" << jbroot << "'; "
           << "test -d \"$JB\" && echo JBROOT_OK || echo JBROOT_MISSING; "
           << "test -f \"$JB/.installed_dopamine\" && echo FLAVOR_DOPAMINE; "
           << "test -f \"$JB/.palera1n-rootless\" && echo FLAVOR_PALERA1N; "
           << "test -f \"$JB/.procursus_strapped\" && echo FLAVOR_PROCURSUS; "
           << "test -x \"$JB/basebin/jbctl\" && echo MARKER_JBCTL; "
           << "test -f \"$JB/usr/bin/dpkg\" && echo MARKER_DPKG || echo MISSING:usr/bin/dpkg; "
           << "test -f \"$JB/usr/bin/bash\" && echo MARKER_BASH; "
           << "test -d \"$JB/Library/LaunchDaemons\" && echo MARKER_LAUNCHD; "
           << "mount 2>/dev/null | grep -E '/var/jb|jbroot' >/dev/null && echo BINDMOUNT; "
           << "test -w /Library/MobileSubstrate 2>/dev/null && echo ROOTFUL_RW";
    return script.str();
}

} /* anonymous */

bool RootlessDelegate::preferRootless(const ExecutionContext& context) {
    if (PP::envFlagEnabled("PURPLEPOIS0N_ROOTLESS")) {
        return true;
    }
    if (PP::envFlagEnabled("PURPLEPOIS0N_ROOTFUL")) {
        return false;
    }
    JailbreakGeneration era = context.jailbreakGeneration;
    if (era == JailbreakGeneration::Unknown) {
        era = detectJailbreakGeneration(context);
    }
    if (era == JailbreakGeneration::Gen6) {
        return true;
    }
    return iosVersionInRange(context.iosVersion, "15.0", "99.0");
}

bool RootlessDelegate::sshConfigured(const ExecutionContext& context) {
    if (PP::envFlagEnabled("PURPLEPOIS0N_NORMAL_SSH")) {
        return !context.udid.empty() || !context.ramdiskConnect.udid.empty();
    }
    const RamdiskConnectOptions& connect = context.ramdiskConnect;
    if (connect.transport == RamdiskTransport::Ssh) {
        return true;
    }
    if (!connect.udid.empty() && connect.sshPort != 0) {
        return true;
    }
    return PP::envOrEmpty("PURPLEPOIS0N_RAMDISK_SSH_PORT") != std::string() ||
           PP::envOrEmpty("PURPLEPOIS0N_RAMDISK_TRANSPORT") == "ssh";
}

RamdiskConnectOptions RootlessDelegate::sshConnectOptions(const ExecutionContext& context) {
    RamdiskConnectOptions opts = context.ramdiskConnect;
    opts.transport = RamdiskTransport::Ssh;
    if (opts.host.empty()) {
        opts.host = "127.0.0.1";
    }
    if (opts.sshPort == 0) {
        opts.sshPort = PP::parsePortEnv("PURPLEPOIS0N_RAMDISK_SSH_PORT", 2222);
    }
    if (opts.udid.empty() && !context.udid.empty()) {
        opts.udid = context.udid;
    }
    opts.autoIproxy = PP::truthyEnv("PURPLEPOIS0N_RAMDISK_AUTO_IPROXY", true);
    return opts;
}

RootlessProbeResult RootlessDelegate::probeDevice(const ExecutionContext& context) {
    RootlessProbeResult result;
    result.jbroot = RootlessLayout::resolveJbroot();

    if (!sshConfigured(context)) {
        Logger::info("  [Rootless] SSH not configured — use --normal-ssh + iproxy, or "
                     "PURPLEPOIS0N_NORMAL_SSH=1");
        return result;
    }

    RamdiskClient client(sshConnectOptions(context));
    std::string probeMsg;
    result.sshReachable = client.probe(&probeMsg);
    if (!result.sshReachable) {
        Logger::warn("  [Rootless] SSH probe failed: " + probeMsg);
        return result;
    }
    Logger::info("  [Rootless] " + probeMsg);

    const RamdiskCommandResult remote = client.exec(buildRemoteProbeScript(result.jbroot));
    if (remote.exitCode != 0) {
        Logger::warn("  [Rootless] remote probe script failed: " + remote.stderrText);
        return result;
    }

    std::istringstream lines(remote.stdoutText);
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            appendMarkerFromLine(line, &result);
        }
    }

    result.layoutComplete =
        result.jbrootExists && result.missingRequired.empty() && !result.foundMarkers.empty();
    return result;
}

void RootlessDelegate::logProbeResult(const RootlessProbeResult& result) {
    Logger::info("  [Rootless] jbroot=" + result.jbroot +
                 " exists=" + (result.jbrootExists ? "yes" : "no") +
                 " layout=" + (result.layoutComplete ? "complete" : "partial/missing"));
    if (!result.bootstrapFlavor.empty()) {
        Logger::info("  [Rootless] flavor=" + result.bootstrapFlavor);
    }
    if (result.bindMountsVisible) {
        Logger::info("  [Rootless] bind mounts visible (expected for rootless)");
    }
    if (result.rootfulWritableSystem) {
        Logger::warn("  [Rootless] /Library/MobileSubstrate writable — rootful/fakefs layout");
    }
    if (!result.foundMarkers.empty()) {
        std::ostringstream oss;
        oss << "  [Rootless] markers:";
        for (size_t i = 0; i < result.foundMarkers.size(); ++i) {
            oss << " " << result.foundMarkers[i];
        }
        Logger::info(oss.str());
    }
    for (size_t i = 0; i < result.missingRequired.size(); ++i) {
        Logger::warn("  [Rootless] missing required: " + result.missingRequired[i]);
    }
}

bool RootlessDelegate::jbHelperConfigured() {
    return JbHelperDelegate::isConfigured();
}

bool RootlessDelegate::palera1nHelperConfigured() {
    return helperExecutable("PURPLEPOIS0N_PALERA1N_HELPER", nullptr);
}

PrimitiveResult RootlessDelegate::runBootstrapHelper(const ExecutionContext& context,
                                                     bool allowMutation) {
    std::string helperPath;
    const char* argsEnv = "PURPLEPOIS0N_JB_HELPER_ARGS";
    const char* logTag = "JB helper";

    if (helperExecutable("PURPLEPOIS0N_PALERA1N_HELPER", &helperPath) &&
        preferRootless(context)) {
        argsEnv = "PURPLEPOIS0N_PALERA1N_HELPER_ARGS";
        logTag = "palera1n helper";
    } else if (helperExecutable("PURPLEPOIS0N_JB_HELPER", &helperPath)) {
        argsEnv = "PURPLEPOIS0N_JB_HELPER_ARGS";
    } else {
        return PrimitiveResult::PrerequisitesMissing;
    }

    return spawnHelper(helperPath, argsEnv, context, allowMutation, logTag);
}

} /* namespace primitives */
} /* namespace PP */
