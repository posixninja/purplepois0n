/*
 * Usbliter8.cpp
 */

#include "Usbliter8.h"
#include "Checkm8.h"
#include "DFUDevice.h"
#include "DeviceManager.h"
#include "Logger.h"
#include "../include/DeviceState.h"

#include <libirecovery.h>

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <unistd.h>

namespace PP {

namespace {

/* A12 (T8020), A13 (T8030), S4/S5 (T8006) — see
 * https://ps.tc/pages/blog-usbliter8.html. A12X/Z are theoretically
 * reachable via the same bug but are not implemented by usbliter8 yet. */
const std::unordered_set<uint32_t> kSupportedCpids = {
    0x8006, 0x8020, 0x8030,
};

bool pathIsExecutable(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    return access(path.c_str(), X_OK) == 0;
}

std::string envPathOrDefault(const char* envName, const char* fallback) {
    const char* fromEnv = std::getenv(envName);
    if (fromEnv != nullptr && fromEnv[0] != '\0') {
        return std::string(fromEnv);
    }
    return std::string(fallback);
}

int runShell(const std::string& command) {
    Logger::debug("Executing: " + command);
    return std::system(command.c_str());
}

const char* actionToString(Usbliter8Action action) {
    return action == Usbliter8Action::Demote ? "demote" : "boot";
}

bool tryUsbliter8ctl(Usbliter8Action action) {
    std::string ctl = envPathOrDefault("PURPLEPOIS0N_USBLITER8CTL", "usbliter8ctl");
    if (!pathIsExecutable(ctl) && access(ctl.c_str(), F_OK) != 0) {
        const char* candidates[] = {
            "/usr/local/bin/usbliter8ctl",
            "/opt/homebrew/bin/usbliter8ctl",
            "usbliter8/usbliter8ctl",
            "../usbliter8/usbliter8ctl",
            "../../usbliter8/usbliter8ctl",
        };
        bool found = false;
        for (const char* candidate : candidates) {
            if (access(candidate, F_OK) == 0) {
                ctl = candidate;
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    const std::string action_str = actionToString(action);
    Logger::info("Using usbliter8ctl for post-exploit control (" + ctl + " " + action_str + ")");
    const std::string cmd = pathIsExecutable(ctl)
                                 ? ("\"" + ctl + "\" " + action_str)
                                 : ("python3 \"" + ctl + "\" " + action_str);
    const int status = runShell(cmd);
    if (status != 0) {
        Logger::error("usbliter8ctl exited with status " + std::to_string(status));
        return false;
    }
    return true;
}

} /* anonymous namespace */

bool Usbliter8::isSupportedCpid(uint32_t cpid) {
    return kSupportedCpids.count(cpid) != 0;
}

bool Usbliter8::serialIndicatesPwned(const std::string& serial) {
    return serial.find("PWND:[usbliter8]") != std::string::npos;
}

std::string Usbliter8::resultToString(Usbliter8Result result) {
    switch (result) {
        case Usbliter8Result::Success: return "Success";
        case Usbliter8Result::UnsupportedDevice: return "Unsupported device";
        case Usbliter8Result::AlreadyPwned: return "Already pwned";
        case Usbliter8Result::RequiresHardwareBridge: return "Requires external RP2350 hardware bridge";
        case Usbliter8Result::ExternalToolMissing: return "usbliter8ctl not found";
        case Usbliter8Result::ExternalToolFailed: return "usbliter8ctl failed";
        case Usbliter8Result::DeviceOpenFailed: return "Could not open DFU device";
        case Usbliter8Result::UnknownError: return "Unknown error";
        default: return "Unknown";
    }
}

Usbliter8DeviceInfo Usbliter8::probe(DFUDevice& device) {
    Usbliter8DeviceInfo info;
    info.serial = device.getSerialNumber();
    info.deviceType = device.getDeviceType();
    info.cpid = device.getCpid();
    info.ecid = device.getEcid();
    info.socName = Checkm8::cpidToSocName(info.cpid);
    info.pwned = Usbliter8::serialIndicatesPwned(info.serial);
    info.supported = Usbliter8::isSupportedCpid(info.cpid);
    return info;
}

static std::string formatCpid(uint32_t cpid) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << cpid;
    return oss.str();
}

Usbliter8Result Usbliter8::runExploit(const Usbliter8DeviceInfo& info, Usbliter8Action action) {
    {
        std::ostringstream oss;
        oss << "CPID: " << formatCpid(info.cpid) << " (" << info.socName << ")";
        Logger::info(oss.str());
    }
    if (info.ecid != 0) {
        std::ostringstream oss;
        oss << "ECID: 0x" << std::hex << std::uppercase << info.ecid;
        Logger::info(oss.str());
    }
    if (!info.deviceType.empty()) {
        Logger::info("Device type: " + info.deviceType);
    }

    if (info.pwned) {
        Logger::info("Device serial already indicates usbliter8 pwned state");
        if (tryUsbliter8ctl(action)) {
            Logger::info(std::string("usbliter8ctl ") + actionToString(action) + " completed");
        } else {
            Logger::warn("usbliter8ctl not available or failed — device is pwned but no "
                         "post-exploit action was run. Install it from "
                         "https://github.com/prdgmshift/usbliter8, ensure it is on PATH, "
                         "or set PURPLEPOIS0N_USBLITER8CTL.");
        }
        return Usbliter8Result::AlreadyPwned;
    }

    if (!info.supported) {
        Logger::error("CPID " + formatCpid(info.cpid) +
                      " is not in the usbliter8-supported list for this build "
                      "(A12/A13/S4/S5).");
        return Usbliter8Result::UnsupportedDevice;
    }

    Logger::error(
        "usbliter8's DMA-underflow delivery cannot be driven from a normal Mac/PC USB host — "
        "it abuses a low-level DWC2 controller bug that a standard host stack can't reach. "
        "You need an RP2350 microcontroller bridge (Waveshare RP2350 USB-A, Pico 2, etc.) "
        "flashed with the usbliter8 firmware: https://github.com/prdgmshift/usbliter8");
    Logger::info(
        "Put the device in DFU, unplug from this host, replug into the RP2350 board, wait "
        "~1s for the exploit, then replug back into this host and re-run to continue.");
    return Usbliter8Result::RequiresHardwareBridge;
}

bool Usbliter8::runUsbliter8(DeviceManager& manager) {
    if (manager.detectDeviceState() != DeviceState::DFU) {
        Logger::error("usbliter8 requires a device in DFU mode");
        return false;
    }

    Usbliter8DeviceInfo info;
    try {
        auto device = manager.getDFUDevice();
        if (!device) {
            Logger::error("Failed to open DFU device");
            return false;
        }
        info = probe(*device);
    } catch (const std::exception& e) {
        Logger::error(std::string("DFU probe failed: ") + e.what());
        return false;
    }

    Logger::info("DFU handle released; checking usbliter8 state");
    const Usbliter8Result result = runExploit(info);
    switch (result) {
        case Usbliter8Result::Success:
        case Usbliter8Result::AlreadyPwned:
            Logger::info("usbliter8: " + resultToString(result));
            return true;
        default:
            Logger::error("usbliter8: " + resultToString(result));
            return false;
    }
}

} /* namespace PP */
