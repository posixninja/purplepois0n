/*
 * RecoveryDevice.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef RECOVERYDEVICE_H_
#define RECOVERYDEVICE_H_

#include <memory>
#include <string>
#include <cstdint>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <libirecovery.h>
#include <cstdlib>

#include <stdint.h>
#include "IRecvProgress.h"
#include "IRecvUtil.h"

namespace PP {

/**
 * @class RecoveryDevice
 * @brief Interface for communicating with iOS devices in recovery mode
 */
class RecoveryDevice {
public:
    explicit RecoveryDevice(uint64_t ecid, IRecvProgressCallback progress = nullptr);

    ~RecoveryDevice() {
        if (mClient != nullptr) {
            irecv_close(mClient);
        }
    }

    std::string getSerialNumber() const {
        const std::string serial = irecv_util::serialFromClient(mClient);
        if (serial.empty()) {
            throw std::runtime_error("Failed to get serial number");
        }
        return serial;
    }

    std::string getDeviceType() const {
        const std::string productType = irecv_util::productTypeFromClient(mClient);
        if (productType.empty()) {
            throw std::runtime_error("Failed to get device type");
        }
        return productType;
    }

    std::string getFirmwareVersion() const {
        char* value = nullptr;
        if (irecv_getenv(mClient, "build-version", &value) == IRECV_E_SUCCESS && value != nullptr) {
            std::string result(value);
            free(value);
            return result;
        }
        return std::string();
    }

    std::vector<uint8_t> readMemory(const uint64_t address, const uint64_t length) const {
        if (length > 0xFFFFu) {
            throw std::runtime_error("Read length exceeds USB control transfer limit");
        }
        std::vector<uint8_t> buffer(length);
        const int transferred = irecv_util::usbMemoryRead(
            mClient, address, &buffer[0], static_cast<uint16_t>(length));
        if (transferred < 0 || static_cast<uint64_t>(transferred) != length) {
            throw std::runtime_error("Failed to read memory");
        }
        return buffer;
    }

    void writeMemory(const uint64_t address, const std::vector<uint8_t>& data) const {
        if (data.size() > 0xFFFFu) {
            throw std::runtime_error("Write length exceeds USB control transfer limit");
        }
        const int transferred = irecv_util::usbMemoryWrite(
            mClient, address, &data[0], static_cast<uint16_t>(data.size()));
        if (transferred < 0 || static_cast<size_t>(transferred) != data.size()) {
            throw std::runtime_error("Failed to write memory");
        }
    }

    void sendCommand(const std::string& command) const {
        if (irecv_send_command(mClient, command.c_str()) != IRECV_E_SUCCESS) {
            throw std::runtime_error("Failed to send command");
        }
    }

    /** send_command → getret → optional recv_buffer. */
    irecv_util::IRecvCommandResult sendCommandWithResponse(const std::string& command,
                                                           uint64_t recvLength = 0) const {
        return irecv_util::sendCommandWithResponse(mClient, command, recvLength);
    }

    uint32_t getCommandReturn() const {
        uint32_t value = 0;
        if (!irecv_util::getCommandReturn(mClient, &value)) {
            throw std::runtime_error("Failed to read command return");
        }
        return value;
    }

    /** Upload signed IMG3/IMG4 component (iBSS, iBEC, etc.) via libirecovery. */
    void sendFile(const std::string& path, unsigned int options = IRECV_SEND_OPT_NONE) const;

    /** Reset USB/iBoot state (`irecv_reset`). */
    void reset() const;

    /** Reboot device from Recovery (`irecv_reboot`). */
    void reboot() const;

    /** Read iBoot environment variable (caller frees not required — copied to string). */
    std::string getEnv(const std::string& name) const;

    uint32_t getCpid() const;
    uint32_t getBoardId() const;
    std::vector<uint8_t> getApNonce() const;

    uint64_t getEcid() const { return mECID; }

    std::vector<uint8_t> receiveResponse(const uint64_t length) const {
        std::vector<uint8_t> buffer(length);
        if (irecv_recv_buffer(mClient, reinterpret_cast<char*>(&buffer[0]), length) != IRECV_E_SUCCESS) {
            throw std::runtime_error("Failed to receive response");
        }
        return buffer;
    }

private:
    irecv_client_t getClient() const {
        irecv_client_t client = nullptr;
        if (irecv_util::openWithEcidRetry(&client, mECID) != IRECV_E_SUCCESS) {
            throw std::runtime_error("Failed to open device");
        }
        return client;
    }

    const uint64_t mECID;
    irecv_client_t const mClient;
    std::unique_ptr<IRecvProgressSubscription> m_progress;
};

} /* namespace PP */

#endif /* RECOVERYDEVICE_H_ */
