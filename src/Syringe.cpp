/*
 * Syringe.cpp
 */

#include "Syringe.h"
#include "DeviceManager.h"
#include "DFUDevice.h"
#include "RecoveryDevice.h"
#include "MobileDevice.h"
#include "Logger.h"
#include "DoctorReporter.h"
#include <sstream>
#include <fstream>
#include <iomanip>

namespace PP {

Syringe::Syringe()
    : mConnected(false), mTransport(SyringeTransport::DFU), mDeviceManager(nullptr) {}

Syringe::~Syringe() {
    if (mConnected) {
        disconnect();
    }
}

bool Syringe::detectDevice(DeviceManager& manager) {
    Logger::info("Syringe: Detecting device...");
    mDeviceManager = &manager;

    // Try DFU first (bootrom context)
    if (manager.detectDeviceState() == DeviceState::DFU) {
        mTransport = SyringeTransport::DFU;
        mConnected = true;
        Logger::debug("Syringe: Device in DFU mode");
        return true;
    }

    // Try recovery mode
    if (manager.detectDeviceState() == DeviceState::Recovery) {
        mTransport = SyringeTransport::Recovery;
        mConnected = true;
        Logger::debug("Syringe: Device in recovery mode");
        return true;
    }

    // Try normal mode
    if (manager.detectDeviceState() == DeviceState::Normal) {
        mTransport = SyringeTransport::Normal;
        mConnected = true;
        Logger::debug("Syringe: Device in normal mode");
        return true;
    }

    Logger::error("Syringe: No device detected");
    return false;
}

bool Syringe::switchTransport(SyringeTransport target) {
    if (!mConnected) {
        Logger::error("Syringe: Not connected");
        return false;
    }

    if (mTransport == target) {
        return true;
    }

    std::ostringstream oss;
    oss << "Syringe: Switching from " << (int)mTransport << " to " << (int)target;
    Logger::debug(oss.str());

    // TODO: Implement transport switching logic
    // For now, just update the transport type
    mTransport = target;
    return true;
}

bool Syringe::queryCpid(uint32_t& cpid) {
    return fetchCpid(cpid);
}

bool Syringe::queryEcid(uint64_t& ecid) {
    return fetchEcid(ecid);
}

bool Syringe::queryBoardConfig(std::string& config) {
    if (!mConnected) {
        return false;
    }

    SyringeRequest req;
    req.cmd = SyringeCommand::GetBoardConfig;
    SyringeResponse resp;

    if (!sendRequest(req, resp)) {
        return false;
    }

    config = std::string((char*)resp.data.data(), resp.data.size());
    return true;
}

bool Syringe::readMemory(uint64_t addr, uint64_t size, std::vector<uint8_t>& out) {
    if (!mConnected) {
        Logger::error("Syringe: Not connected");
        return false;
    }

    switch (mTransport) {
        case SyringeTransport::DFU:
            return doDFURead(addr, size, out);
        case SyringeTransport::Recovery:
            return doRecoveryRead(addr, size, out);
        case SyringeTransport::Normal:
            Logger::error("Syringe: Cannot read memory in normal mode");
            return false;
    }

    return false;
}

bool Syringe::writeMemory(uint64_t addr, const std::vector<uint8_t>& data) {
    if (!mConnected) {
        Logger::error("Syringe: Not connected");
        return false;
    }

    switch (mTransport) {
        case SyringeTransport::DFU:
            return doDFUWrite(addr, data);
        case SyringeTransport::Recovery:
            return doRecoveryWrite(addr, data);
        case SyringeTransport::Normal:
            Logger::error("Syringe: Cannot write memory in normal mode");
            return false;
    }

    return false;
}

bool Syringe::executeCode(uint64_t addr, const std::vector<uint8_t>& code) {
    if (!mConnected) {
        Logger::error("Syringe: Not connected");
        return false;
    }

    // TODO: Implement code execution via exploit context
    Logger::debug("Syringe: Code execution pending implementation");
    return true;
}

bool Syringe::callFunction(uint64_t funcAddr, const std::vector<uint64_t>& args,
                           uint64_t& result) {
    if (!mConnected) {
        Logger::error("Syringe: Not connected");
        return false;
    }

    // TODO: Call function with args and capture result
    Logger::debug("Syringe: Function calling pending implementation");
    return true;
}

bool Syringe::sendFile(const std::string& filepath, uint64_t addr) {
    if (!mConnected) {
        Logger::error("Syringe: Not connected");
        return false;
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        Logger::error("Syringe: Cannot open file");
        return false;
    }

    // Read file contents
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> fileData(fileSize);
    file.read((char*)fileData.data(), fileSize);
    file.close();

    std::ostringstream oss;
    oss << "Syringe: Sending " << fileSize << " bytes to 0x" << std::hex << addr;
    Logger::debug(oss.str());

    return writeMemory(addr, fileData);
}

bool Syringe::receiveFile(const std::string& filepath, uint64_t addr, uint64_t size) {
    if (!mConnected) {
        Logger::error("Syringe: Not connected");
        return false;
    }

    std::vector<uint8_t> data;
    if (!readMemory(addr, size, data)) {
        Logger::error("Syringe: Failed to read memory for file receive");
        return false;
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        Logger::error("Syringe: Cannot open file for writing");
        return false;
    }

    file.write((char*)data.data(), data.size());
    file.close();

    std::ostringstream oss;
    oss << "Syringe: Received " << size << " bytes from 0x" << std::hex << addr;
    Logger::debug(oss.str());

    return true;
}

bool Syringe::sendRequest(const SyringeRequest& req, SyringeResponse& resp) {
    if (!mConnected) {
        resp.success = false;
        resp.error = "Not connected";
        return false;
    }

    if (!validateRequest(req)) {
        resp.success = false;
        resp.error = "Invalid request";
        return false;
    }

    logRequest(req);

    // Route to appropriate handler based on command
    switch (req.cmd) {
        case SyringeCommand::ReadMemory: {
            std::vector<uint8_t> data;
            resp.success = readMemory(req.addr, req.size, data);
            if (resp.success) {
                resp.data = data;
                resp.status = 0;
            }
            break;
        }

        case SyringeCommand::WriteMemory: {
            resp.success = writeMemory(req.addr, req.data);
            resp.status = resp.success ? 0 : 1;
            break;
        }

        case SyringeCommand::GetCpid: {
            uint32_t cpid = 0;
            resp.success = fetchCpid(cpid);
            if (resp.success) {
                resp.data.resize(4);
                resp.data[0] = static_cast<uint8_t>((cpid >> 24) & 0xFF);
                resp.data[1] = static_cast<uint8_t>((cpid >> 16) & 0xFF);
                resp.data[2] = static_cast<uint8_t>((cpid >> 8) & 0xFF);
                resp.data[3] = static_cast<uint8_t>(cpid & 0xFF);
                resp.status = 0;
            }
            break;
        }

        case SyringeCommand::GetEcid: {
            uint64_t ecid = 0;
            resp.success = fetchEcid(ecid);
            if (resp.success) {
                resp.data.resize(8);
                for (size_t i = 0; i < 8; ++i) {
                    resp.data[i] = static_cast<uint8_t>((ecid >> (56 - i * 8)) & 0xFF);
                }
                resp.status = 0;
            }
            break;
        }

        default:
            resp.success = false;
            resp.error = "Unknown command";
            break;
    }

    logResponse(resp);
    return resp.success;
}

bool Syringe::runCommand(const std::string& command, SyringeResponse& resp, const uint64_t recvLength) {
    if (!mConnected) {
        resp.success = false;
        resp.error = "Not connected";
        return false;
    }
    return runTransportCommand(command, resp, recvLength);
}

bool Syringe::reconnect() {
    if (mDeviceManager && detectDevice(*mDeviceManager)) {
        mConnected = true;
        Logger::info("Syringe: Reconnected");
        return true;
    }

    mConnected = false;
    return false;
}

void Syringe::disconnect() {
    Logger::info("Syringe: Disconnecting");
    mConnected = false;
    mDeviceManager = nullptr;
}

bool Syringe::doDFURead(uint64_t addr, uint64_t size, std::vector<uint8_t>& out) {
    if (!mDeviceManager) {
        Logger::error("Syringe: Device manager not available for DFU read");
        return false;
    }

    try {
        auto dfu = mDeviceManager->getDFUDevice();
        if (!dfu) {
            Logger::error("Syringe: Failed to get DFU device");
            return false;
        }

        out = dfu->readMemory(addr, size);

        std::ostringstream oss;
        oss << "Syringe DFU: Read " << out.size() << " bytes from 0x" << std::hex << addr;
        Logger::debug(oss.str());

        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("Syringe DFU read failed: ") + e.what());
        return false;
    }
}

bool Syringe::doDFUWrite(uint64_t addr, const std::vector<uint8_t>& data) {
    if (!mDeviceManager) {
        Logger::error("Syringe: Device manager not available for DFU write");
        return false;
    }

    if (data.empty()) {
        Logger::warn("Syringe: Attempting to write empty buffer");
        return false;
    }

    try {
        auto dfu = mDeviceManager->getDFUDevice();
        if (!dfu) {
            Logger::error("Syringe: Failed to get DFU device");
            return false;
        }

        dfu->writeMemory(addr, data);

        std::ostringstream oss;
        oss << "Syringe DFU: Wrote " << data.size() << " bytes to 0x" << std::hex << addr;
        Logger::info(oss.str());

        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("Syringe DFU write failed: ") + e.what());
        return false;
    }
}

bool Syringe::doRecoveryRead(uint64_t addr, uint64_t size, std::vector<uint8_t>& out) {
    if (!mDeviceManager) {
        Logger::error("Syringe: Device manager not available for recovery read");
        return false;
    }

    try {
        uint64_t ecid = 0;
        if (!fetchEcid(ecid)) {
            Logger::error("Syringe: Failed to query ECID for recovery device");
            return false;
        }

        auto recovery = mDeviceManager->getRecoveryDevice(ecid);
        if (!recovery) {
            Logger::error("Syringe: Failed to get recovery device");
            return false;
        }

        out = recovery->readMemory(addr, size);

        std::ostringstream oss;
        oss << "Syringe Recovery: Read " << out.size() << " bytes from 0x" << std::hex << addr;
        Logger::debug(oss.str());

        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("Syringe recovery read failed: ") + e.what());
        return false;
    }
}

bool Syringe::doRecoveryWrite(uint64_t addr, const std::vector<uint8_t>& data) {
    if (!mDeviceManager) {
        Logger::error("Syringe: Device manager not available for recovery write");
        return false;
    }

    try {
        uint64_t ecid = 0;
        if (!fetchEcid(ecid)) {
            Logger::error("Syringe: Failed to query ECID for recovery device");
            return false;
        }

        auto recovery = mDeviceManager->getRecoveryDevice(ecid);
        if (!recovery) {
            Logger::error("Syringe: Failed to get recovery device");
            return false;
        }

        recovery->writeMemory(addr, data);

        std::ostringstream oss;
        oss << "Syringe Recovery: Wrote " << data.size() << " bytes to 0x" << std::hex << addr;
        Logger::debug(oss.str());

        return true;
    } catch (const std::exception& e) {
        Logger::error(std::string("Syringe recovery write failed: ") + e.what());
        return false;
    }
}

bool Syringe::fetchCpid(uint32_t& cpid) {
    if (!mConnected || mDeviceManager == nullptr) {
        return false;
    }
    try {
        switch (mTransport) {
            case SyringeTransport::DFU: {
                auto dfu = mDeviceManager->getDFUDevice();
                if (!dfu) {
                    return false;
                }
                cpid = dfu->getCpid();
                return cpid != 0;
            }
            case SyringeTransport::Recovery: {
                uint64_t ecid = 0;
                if (!fetchEcid(ecid)) {
                    return false;
                }
                auto recovery = mDeviceManager->getRecoveryDevice(ecid);
                if (!recovery) {
                    return false;
                }
                cpid = recovery->getCpid();
                return cpid != 0;
            }
            case SyringeTransport::Normal:
                return false;
        }
    } catch (const std::exception& e) {
        Logger::error(std::string("Syringe CPID query failed: ") + e.what());
        return false;
    }
    return false;
}

bool Syringe::fetchEcid(uint64_t& ecid) {
    if (!mConnected || mDeviceManager == nullptr) {
        return false;
    }
    try {
        switch (mTransport) {
            case SyringeTransport::DFU: {
                auto dfu = mDeviceManager->getDFUDevice();
                if (!dfu) {
                    return false;
                }
                ecid = dfu->getEcid();
                return ecid != 0;
            }
            case SyringeTransport::Recovery:
                ecid = mDeviceManager->getRecoveryEcid();
                return ecid != 0;
            case SyringeTransport::Normal:
                return false;
        }
    } catch (const std::exception& e) {
        Logger::error(std::string("Syringe ECID query failed: ") + e.what());
        return false;
    }
    return false;
}

bool Syringe::runTransportCommand(const std::string& command,
                                  SyringeResponse& resp,
                                  const uint64_t recvLength) {
    resp = SyringeResponse();
    if (!mDeviceManager) {
        resp.success = false;
        resp.error = "No device manager";
        return false;
    }

    const char* transport = "unknown";
    switch (mTransport) {
        case SyringeTransport::DFU:
            transport = "dfu";
            break;
        case SyringeTransport::Recovery:
            transport = "recovery";
            break;
        case SyringeTransport::Normal:
            transport = "normal";
            break;
    }

    auto& reporter = DoctorReporter::instance();
    if (reporter.enabled()) {
        reporter.syringeRequest(transport, command);
    }

    try {
        if (mTransport == SyringeTransport::Normal) {
            resp.success = false;
            resp.error = "Commands not supported in normal mode";
            if (reporter.enabled()) {
                reporter.syringeResponse(transport, false, 1, resp.error);
            }
            return false;
        }

        irecv_util::IRecvCommandResult result;
        if (mTransport == SyringeTransport::DFU) {
            auto dfu = mDeviceManager->getDFUDevice();
            if (!dfu) {
                resp.success = false;
                resp.error = "DFU device unavailable";
                if (reporter.enabled()) {
                    reporter.syringeResponse(transport, false, 1, resp.error);
                }
                return false;
            }
            result = dfu->sendCommandWithResponse(command, recvLength);
        } else {
            uint64_t ecid = 0;
            if (!fetchEcid(ecid)) {
                resp.success = false;
                resp.error = "Recovery ECID unavailable";
                if (reporter.enabled()) {
                    reporter.syringeResponse(transport, false, 1, resp.error);
                }
                return false;
            }
            auto recovery = mDeviceManager->getRecoveryDevice(ecid);
            if (!recovery) {
                resp.success = false;
                resp.error = "Recovery device unavailable";
                if (reporter.enabled()) {
                    reporter.syringeResponse(transport, false, 1, resp.error);
                }
                return false;
            }
            result = recovery->sendCommandWithResponse(command, recvLength);
        }

        resp.success = result.success;
        resp.status = result.returnCode;
        resp.data = result.buffer;
        if (!result.success) {
            resp.error = result.error.empty() ? "command failed" : result.error;
        }

        if (reporter.enabled()) {
            std::ostringstream detail;
            detail << "status=" << resp.status;
            if (!resp.data.empty()) {
                detail << " bytes=" << resp.data.size();
            }
            if (!resp.error.empty()) {
                detail << " err=" << resp.error;
            }
            reporter.syringeResponse(transport, resp.success, resp.status, detail.str());
        }

        logResponse(resp);
        return resp.success;
    } catch (const std::exception& e) {
        resp.success = false;
        resp.error = e.what();
        if (reporter.enabled()) {
            reporter.syringeResponse(transport, false, 1, resp.error);
        }
        return false;
    }
}

bool Syringe::validateRequest(const SyringeRequest& req) const {
    // Basic validation
    if (req.cmd == SyringeCommand::ReadMemory || req.cmd == SyringeCommand::WriteMemory) {
        if (req.size == 0) {
            return false;
        }
    }

    return true;
}

void Syringe::logRequest(const SyringeRequest& req) {
    std::ostringstream oss;
    oss << "Syringe TX: " << (int)req.cmd;
    if (!req.tag.empty()) {
        oss << " [" << req.tag << "]";
    }
    Logger::debug(oss.str());
}

void Syringe::logResponse(const SyringeResponse& resp) {
    std::ostringstream oss;
    oss << "Syringe RX: status=" << resp.status << " success=" << resp.success;
    if (!resp.error.empty()) {
        oss << " err=" << resp.error;
    }
    Logger::debug(oss.str());
}

} /* namespace PP */
