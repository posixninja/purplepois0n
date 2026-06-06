/*
 * SideloadPrimitive.cpp
 */

#include "primitives/SideloadPrimitive.h"
#include "InstproxyService.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace PP {
namespace primitives {

const char* SideloadPrimitive::name() const { return "sideload-install"; }

PrimitiveCategory SideloadPrimitive::category() const {
    return PrimitiveCategory::Injection;
}

std::vector<PrimitiveOperation> SideloadPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> SideloadPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Normal};
}

bool SideloadPrimitive::canRun(const ExecutionContext& context) const {
    if (context.deviceState != DeviceState::Normal || context.udid.empty()) {
        return false;
    }
    if (!context.ipaInstallPath.empty()) {
        return true;
    }
    const char* env = std::getenv("PURPLEPOIS0N_INSTALL_IPA");
    return env != nullptr && env[0] != '\0';
}

namespace {

std::string resolveIpaPath(const ExecutionContext& context) {
    if (!context.ipaInstallPath.empty()) {
        return context.ipaInstallPath;
    }
    const char* env = std::getenv("PURPLEPOIS0N_INSTALL_IPA");
    return (env != nullptr) ? std::string(env) : std::string();
}

bool runIdeviceinstaller(const std::string& udid, const std::string& ipaPath) {
    const char* tool = std::getenv("PURPLEPOIS0N_IDEVICEINSTALLER");
    if (tool == nullptr || tool[0] == '\0') {
        tool = "ideviceinstaller";
    }
    if (access(tool, X_OK) != 0) {
        const std::string found = ToolRunner::findExecutable(tool);
        if (found.empty()) {
            return false;
        }
    }

    std::vector<std::string> argv;
    argv.push_back(tool);
    argv.push_back("-u");
    argv.push_back(udid);
    argv.push_back("-i");
    argv.push_back(ipaPath);
    Logger::info("  [Sideload] ideviceinstaller -i " + ipaPath);
    return ToolRunner::run(argv).exitCode == 0;
}

} /* anonymous */

PrimitiveResult SideloadPrimitive::execute(ExecutionContext& context) {
    const std::string ipaPath = resolveIpaPath(context);
    if (ipaPath.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }

    Logger::info("  [Sideload] IPA install target: " + ipaPath);
    Logger::info("  [Sideload] stock iOS rejects ad-hoc IPAs unless jailbroken + trust cache");

    if (!context.allowMutation) {
        InstproxyService service(context.udid);
        std::string error;
        if (service.probe(&error)) {
            Logger::info("  [Sideload] instproxy connected — would install " + ipaPath);
        } else {
            Logger::warn("  [Sideload] instproxy probe failed: " + error);
            Logger::info("  [Sideload] would install " + ipaPath + " when device is trusted");
        }
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [Sideload] install requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    const char* useInstaller = std::getenv("PURPLEPOIS0N_IDEVICEINSTALLER");
    if (useInstaller != nullptr && useInstaller[0] != '\0') {
        return runIdeviceinstaller(context.udid, ipaPath) ? PrimitiveResult::Success
                                                          : PrimitiveResult::Failed;
    }

    InstproxyService service(context.udid);
    std::string error;
    if (service.install(ipaPath, &error)) {
        return PrimitiveResult::Success;
    }
    Logger::error("  [Sideload] " + error);
    return PrimitiveResult::Failed;
}

} /* namespace primitives */
} /* namespace PP */
