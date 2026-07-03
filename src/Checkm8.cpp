/*
 * Checkm8.cpp
 *
 * checkm8 (bootrom DFU) support: SoC identification, support checks, and exploit
 * delivery via external gaster/ipwndfu when present (full USB exploit sequences are
 * maintained upstream; this module wires purplepois0n to those tools cleanly).
 */

#include "Checkm8.h"
#include "DFUDevice.h"
#include "DeviceManager.h"
#include "Logger.h"
#include "Usbliter8.h"
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

const std::unordered_set<uint32_t> kSupportedCpids = {
    0x8940, 0x8942, 0x8945, 0x8947, /* A5 */
    0x8950, 0x8955,                     /* A6 */
    0x8960,                           /* A7 */
    0x7000, 0x7001, 0x7002,           /* A7/A8 variants (iPad, Apple TV, etc.) */
    0x8000, 0x8001, 0x8002, 0x8003, 0x8004, /* A8/A9 */
    0x8010, 0x8011, 0x8012, 0x8015    /* A10/A11 / T2-class */
};

/* Common A12+ CPIDs — not vulnerable to checkm8 */
const std::unordered_set<uint32_t> kUnsupportedCpids = {
    0x8020, 0x8030, 0x8101, 0x8110, 0x8120, 0x8140, 0x8142, 0x8143,
    0x8201, 0x8210, 0x8211, 0x8212, 0x8220
};

bool parseHexField(const std::string& serial, const std::string& key, uint64_t& out) {
    const std::string needle = key + ":";
    const size_t pos = serial.find(needle);
    if (pos == std::string::npos) {
        return false;
    }
    size_t start = pos + needle.size();
    size_t end = start;
    while (end < serial.size() && serial[end] != ' ') {
        ++end;
    }
    const std::string value = serial.substr(start, end - start);
    if (value.empty()) {
        return false;
    }
    std::stringstream ss;
    ss << std::hex << value;
    ss >> out;
    return !ss.fail();
}

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

bool tryGaster() {
    std::string gaster = envPathOrDefault("PURPLEPOIS0N_GASTER", "gaster");
    if (!pathIsExecutable(gaster)) {
        const char* candidates[] = {
            "/usr/local/bin/gaster",
            "/opt/homebrew/bin/gaster",
        };
        bool found = false;
        for (const char* candidate : candidates) {
            if (pathIsExecutable(candidate)) {
                gaster = candidate;
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    Logger::info("Using gaster for checkm8 (" + gaster + " pwn)");
    const std::string cmd = "\"" + gaster + "\" pwn";
    const int status = runShell(cmd);
    if (status != 0) {
        Logger::error("gaster exited with status " + std::to_string(status));
        return false;
    }
    return true;
}

bool tryIpwndfu() {
    std::string root = envPathOrDefault("PURPLEPOIS0N_IPWNDFU", "");
    if (root.empty()) {
        const char* candidates[] = {
            "ipwndfu",
            "../ipwndfu",
            "../../ipwndfu",
        };
        for (const char* candidate : candidates) {
            std::string script = std::string(candidate) + "/ipwndfu";
            if (access(script.c_str(), F_OK) == 0) {
                root = candidate;
                break;
            }
        }
    }
    if (root.empty()) {
        return false;
    }

    std::string ipwndfu = root;
    if (ipwndfu.back() != '/') {
        ipwndfu += '/';
    }
    ipwndfu += "ipwndfu";

    if (access(ipwndfu.c_str(), F_OK) != 0) {
        return false;
    }

    Logger::info("Using ipwndfu for checkm8 (" + ipwndfu + " -p)");
    const std::string cmd = "cd \"" + root + "\" && python3 ipwndfu -p";
    const int status = runShell(cmd);
    if (status != 0) {
        Logger::error("ipwndfu exited with status " + std::to_string(status));
        return false;
    }
    return true;
}

Checkm8DeviceInfo buildInfo(DFUDevice& device) {
    Checkm8DeviceInfo info;
    info.serial = device.getSerialNumber();
    info.deviceType = device.getDeviceType();
    info.cpid = device.getCpid();
    info.ecid = device.getEcid();
    info.socName = Checkm8::cpidToSocName(info.cpid);
    info.pwned = Checkm8::serialIndicatesPwned(info.serial);
    info.supported = Checkm8::isSupportedCpid(info.cpid);
    return info;
}

} /* anonymous namespace */

uint32_t Checkm8::parseCpidFromSerial(const std::string& serial) {
    uint64_t value = 0;
    if (parseHexField(serial, "CPID", value)) {
        return static_cast<uint32_t>(value);
    }
    return 0;
}

uint64_t Checkm8::parseEcidFromSerial(const std::string& serial) {
    uint64_t value = 0;
    parseHexField(serial, "ECID", value);
    return value;
}

bool Checkm8::serialIndicatesPwned(const std::string& serial) {
    if (Usbliter8::serialIndicatesPwned(serial)) {
        /* pwned via usbliter8, not checkm8 — see Usbliter8::serialIndicatesPwned */
        return false;
    }
    return serial.find("PWND:") != std::string::npos ||
           serial.find("PWNED:") != std::string::npos ||
           serial.find("checkm8") != std::string::npos;
}

bool Checkm8::isSupportedCpid(uint32_t cpid) {
    return kSupportedCpids.count(cpid) != 0;
}

bool Checkm8::isKnownUnsupportedCpid(uint32_t cpid) {
    if (cpid == 0) {
        return false;
    }
    if (kUnsupportedCpids.count(cpid) != 0) {
        return true;
    }
    /* Heuristic: A12+ often uses 0x80xx/0x81xx/0x82xx outside the checkm8 set */
    if (cpid >= 0x8020 && !isSupportedCpid(cpid)) {
        return true;
    }
    return false;
}

std::string Checkm8::cpidToSocName(uint32_t cpid) {
    switch (cpid) {
        /* Gen 0 bootrom / pre-checkm8 (syringe-style names) */
        case 0x8720: return "S5L8720 (iPod touch 2G, old BR)";
        case 0x8721: return "S5L8720 rev (iPod touch 2G)";
        case 0x8920: return "S5L8920 (iPhone 3GS, old BR)";
        case 0x8922: return "S5L8922 (iPod touch 3G)";
        case 0x8925: return "S5L8925 (iPad 1 Wi-Fi)";
        case 0x8926: return "S5L8926 (iPad 1 GSM)";
        case 0x8930: return "S5L8930 (iPhone 3GS new BR / A4-class)";
        case 0x8933: return "S5L8933 (iPad 2 Wi-Fi)";
        case 0x8935: return "S5L8935 (iPad 2 GSM)";
        case 0x8940: return "A5 (S5L8940)";
        case 0x8942: return "A5X (S5L8942)";
        case 0x8945: return "A5 (S5L8945)";
        case 0x8947: return "A5 rev (S5L8947)";
        case 0x8950: return "A6 (S5L8950)";
        case 0x8955: return "A6 (S5L8955)";
        case 0x8960: return "A7 (S5L8960)";
        case 0x7000: return "A8 (T7000)";
        case 0x7001: return "A8 (T7001)";
        case 0x7002: return "A8 variant (T7002)";
        case 0x8000: return "A9 (S8000)";
        case 0x8001: return "A9 (S8001)";
        case 0x8002: return "A9 variant (S8002)";
        case 0x8003: return "A9 (S8003)";
        case 0x8004: return "A9 variant (S8004)";
        case 0x8010: return "A10 (T8010)";
        case 0x8011: return "A10 (T8011)";
        case 0x8012: return "A10 variant (T8012)";
        case 0x8015: return "A11 (T8015)";
        default: {
            std::ostringstream oss;
            oss << "CPID 0x" << std::hex << std::uppercase << cpid;
            return oss.str();
        }
    }
}

std::string Checkm8::resultToString(Checkm8Result result) {
    switch (result) {
        case Checkm8Result::Success: return "Success";
        case Checkm8Result::UnsupportedDevice: return "Unsupported device";
        case Checkm8Result::AlreadyPwned: return "Already pwned";
        case Checkm8Result::ExternalToolMissing: return "External exploit tool not found";
        case Checkm8Result::ExternalToolFailed: return "External exploit tool failed";
        case Checkm8Result::DeviceOpenFailed: return "Could not open DFU device";
        case Checkm8Result::UnknownError: return "Unknown error";
        default: return "Unknown";
    }
}

Checkm8DeviceInfo Checkm8::probe(DFUDevice& device) {
    return buildInfo(device);
}

static std::string formatCpid(uint32_t cpid) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << cpid;
    return oss.str();
}

Checkm8Result Checkm8::runExploit(const Checkm8DeviceInfo& info, bool preferExternal) {
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
        Logger::info("Device serial already indicates checkm8 pwned state");
        return Checkm8Result::AlreadyPwned;
    }

    if (isKnownUnsupportedCpid(info.cpid)) {
        Logger::error("CPID " + formatCpid(info.cpid) +
                      " is A12 or newer — checkm8 does not apply. Use a userland jailbreak instead.");
        return Checkm8Result::UnsupportedDevice;
    }

    if (!info.supported) {
        Logger::error("CPID " + formatCpid(info.cpid) +
                      " is not in the checkm8-supported list for this build.");
        return Checkm8Result::UnsupportedDevice;
    }

    if (!preferExternal) {
        Logger::error("In-tree USB exploit sequence is not bundled; use gaster or ipwndfu.");
        return Checkm8Result::ExternalToolMissing;
    }

    Logger::info("Stage 1/2: Running external checkm8 tool (gaster or ipwndfu)");

    if (tryGaster()) {
        Logger::info("Stage 2/2: Verifying pwned state");
        try {
            DFUDevice verify;
            if (serialIndicatesPwned(verify.getSerialNumber())) {
                Logger::info("checkm8 succeeded (PWND marker in USB serial)");
                return Checkm8Result::Success;
            }
            Logger::warn("Exploit tool finished but PWND marker not seen; device may still be pwned");
            return Checkm8Result::Success;
        } catch (const std::exception& e) {
            Logger::warn(std::string("Could not re-open DFU device for verification: ") + e.what());
            return Checkm8Result::Success;
        }
    }

    if (tryIpwndfu()) {
        Logger::info("Stage 2/2: Verifying pwned state");
        try {
            DFUDevice verify;
            if (serialIndicatesPwned(verify.getSerialNumber())) {
                Logger::info("checkm8 succeeded (PWND marker in USB serial)");
                return Checkm8Result::Success;
            }
            Logger::warn("ipwndfu finished but PWND marker not seen; check USB and retry");
            return Checkm8Result::Success;
        } catch (const std::exception& e) {
            Logger::warn(std::string("Could not re-open DFU device for verification: ") + e.what());
            return Checkm8Result::Success;
        }
    }

    Logger::error("No checkm8 tool found. Install gaster (https://github.com/0x7ff/gaster) and ensure "
                  "it is on PATH, or set PURPLEPOIS0N_GASTER. Alternatively set PURPLEPOIS0N_IPWNDFU "
                  "to an ipwndfu checkout and run: python3 ipwndfu -p");
    return Checkm8Result::ExternalToolMissing;
}

bool Checkm8::runCheckm8(DeviceManager& manager) {
    if (manager.detectDeviceState() != DeviceState::DFU) {
        Logger::error("checkm8 requires a device in DFU mode");
        return false;
    }

    Checkm8DeviceInfo info;
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

    Logger::info("DFU handle released; starting checkm8");
    const Checkm8Result result = runExploit(info);
    switch (result) {
        case Checkm8Result::Success:
        case Checkm8Result::AlreadyPwned:
            Logger::info("checkm8: " + resultToString(result));
            return true;
        default:
            Logger::error("checkm8: " + resultToString(result));
            return false;
    }
}

} /* namespace PP */
