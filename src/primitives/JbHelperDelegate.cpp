/*
 * JbHelperDelegate.cpp
 */

#include "primitives/JbHelperDelegate.h"
#include "primitives/RootlessDelegate.h"
#include "primitives/RootlessLayout.h"
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

} /* anonymous namespace */

bool JbHelperDelegate::isConfigured() {
    const char* helper = std::getenv("PURPLEPOIS0N_JB_HELPER");
    return helper != nullptr && helper[0] != '\0' && access(helper, X_OK) == 0;
}

PrimitiveResult JbHelperDelegate::run(const ExecutionContext& context, bool allowMutation) {
    const char* helperPath = std::getenv("PURPLEPOIS0N_JB_HELPER");
    if (helperPath == nullptr || helperPath[0] == '\0') {
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (!allowMutation) {
        if (access(helperPath, X_OK) == 0) {
            Logger::info(std::string("  [Gen6]   JB helper configured: ") + helperPath);
        } else {
            Logger::warn(std::string("  [Gen6]   PURPLEPOIS0N_JB_HELPER not executable: ") +
                         helperPath);
        }
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [Gen6]   JB helper spawn requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    std::vector<std::string> argv;
    argv.push_back(helperPath);

    if (RootlessDelegate::preferRootless(context)) {
        argv.push_back("--rootless");
        argv.push_back("--jbroot");
        argv.push_back(RootlessLayout::resolveJbroot());
    }

    const char* extraArgs = std::getenv("PURPLEPOIS0N_JB_HELPER_ARGS");
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

    Logger::info(std::string("  [Gen6]   spawning JB helper: ") + helperPath);
    const CommandResult result = ToolRunner::run(argv);
    if (result.exitCode == 0) {
        Logger::info("  [Gen6]   JB helper exited 0");
        return PrimitiveResult::Success;
    }

    Logger::error("  [Gen6]   JB helper exit " + std::to_string(result.exitCode));
    if (!result.stderrText.empty()) {
        Logger::warn("  [Gen6]   stderr: " + result.stderrText);
    }
    return PrimitiveResult::Failed;
}

} /* namespace primitives */
} /* namespace PP */
