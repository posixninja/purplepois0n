/*
 * DFUDevice.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef DFUDEVICE_H_
#define DFUDEVICE_H_

#include <cstdint>
#include <functional>
#include <libirecovery.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "IRecvProgress.h"
#include "IRecvUtil.h"

namespace PP {

/**
 * @class DFUDevice
 * @brief Interface for communicating with iOS devices in DFU (Device Firmware Update) mode
 */
class DFUDevice {
public:
    explicit DFUDevice(IRecvProgressCallback progress = nullptr);

    ~DFUDevice();

    DFUDevice(const DFUDevice&) = delete;
    DFUDevice& operator=(const DFUDevice&) = delete;

    std::string getSerialNumber() const;
    std::string getDeviceType() const;
    std::string getFirmwareVersion() const;

    std::vector<uint8_t> readMemory(uint64_t address, uint64_t length) const;
    void writeMemory(uint64_t address, const std::vector<uint8_t>& data) const;

    void sendCommand(const std::string& command) const;
    /** send_command → getret → optional recv_buffer. */
    irecv_util::IRecvCommandResult sendCommandWithResponse(const std::string& command,
                                                           uint64_t recvLength = 0) const;
    uint32_t getCommandReturn() const;
    void sendFile(const std::string& path, unsigned int options = IRECV_SEND_OPT_NONE) const;

    std::vector<uint8_t> receiveResponse(uint64_t length) const;

    bool isInDFUMode() const;
    uint32_t getCpid() const;
    uint64_t getEcid() const;

    /** Raw libirecovery handle (owned by this object). */
    irecv_client_t client() const { return mClient; }

private:
    irecv_client_t openClient() const;

    irecv_client_t mClient;
    std::unique_ptr<IRecvProgressSubscription> m_progress;
};

} /* namespace PP */

#endif /* DFUDEVICE_H_ */
