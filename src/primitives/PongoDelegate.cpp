/*
 * PongoDelegate.cpp
 */

#include "primitives/PongoDelegate.h"
#include "pongo/PongoDevice.h"
#include "EnvUtil.h"
#include "Logger.h"

#include <unistd.h>

namespace PP {
namespace primitives {

namespace {

bool pathIsExecutable(const std::string& path) {
    return !path.empty() && access(path.c_str(), X_OK) == 0;
}

int runShell(const std::string& cmd) {
    Logger::debug("Executing: " + cmd);
    return std::system(cmd.c_str());
}

} /* anonymous namespace */

std::string PongoDelegate::resolveCheckra1nPath() {
    const std::string fromEnv = PP::envOrEmpty("PURPLEPOIS0N_CHECKRA1N");
    if (pathIsExecutable(fromEnv)) {
        return fromEnv;
    }
    const char* candidates[] = {
        "/Applications/checkra1n.app/Contents/MacOS/checkra1n",
        "/usr/local/bin/checkra1n",
        "/opt/homebrew/bin/checkra1n",
        "checkra1n",
    };
    for (const char* candidate : candidates) {
        if (pathIsExecutable(candidate)) {
            return std::string(candidate);
        }
    }
    return fromEnv.empty() ? std::string("checkra1n") : fromEnv;
}

bool PongoDelegate::isPongoPresent() {
    if (!pongoLibusbAvailable()) {
        return false;
    }
    return PongoDevice::isPresent();
}

PrimitiveResult PongoDelegate::spawnCheckra1nShell(bool allowMutation) {
    const std::string checkra1n = resolveCheckra1nPath();
    if (!allowMutation) {
        Logger::info("  [Pongo] would spawn: \"" + checkra1n + "\" -cp (device must be in DFU)");
        return PrimitiveResult::Success;
    }
    if (!pathIsExecutable(checkra1n)) {
        Logger::error("  [Pongo] checkra1n not found — set PURPLEPOIS0N_CHECKRA1N");
        return PrimitiveResult::PrerequisitesMissing;
    }
    Logger::info("  [Pongo] spawning checkra1n -cp — place device in DFU if not already pwned");
    const std::string cmd = "\"" + checkra1n + "\" -cp";
    const int status = runShell(cmd);
    if (status != 0) {
        Logger::warn("  [Pongo] checkra1n exited with status " + std::to_string(status));
        return PrimitiveResult::Failed;
    }
    return PrimitiveResult::Success;
}

PrimitiveResult PongoDelegate::probeShell(std::string* stdoutText) {
    if (!pongoLibusbAvailable()) {
        Logger::warn("  [Pongo] rebuild with libusb (brew install libusb) for USB probe");
        return PrimitiveResult::PrerequisitesMissing;
    }
    PongoDevice dev;
    if (!dev.open()) {
        return PrimitiveResult::PrerequisitesMissing;
    }
    if (!dev.sendCommand("version")) {
        return PrimitiveResult::TransportError;
    }
    std::string out;
    if (!dev.getStdOut(&out)) {
        return PrimitiveResult::TransportError;
    }
    if (stdoutText != nullptr) {
        *stdoutText = out;
    }
    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
