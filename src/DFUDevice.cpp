/*
 * DFUDevice.cpp
 */

#include "DFUDevice.h"

namespace PP {

DFUDevice::DFUDevice(IRecvProgressCallback progress)
    : mClient(openClient()) {
    if (progress) {
        m_progress = std::make_unique<IRecvProgressSubscription>(mClient, std::move(progress));
    }
}

DFUDevice::~DFUDevice() {
    if (mClient != nullptr) {
        irecv_close(mClient);
        mClient = nullptr;
    }
}

irecv_client_t DFUDevice::openClient() const {
    irecv_client_t client = nullptr;
    if (irecv_util::openWithEcidRetry(&client, 0) != IRECV_E_SUCCESS) {
        throw std::runtime_error("Failed to open DFU device");
    }
    return client;
}

std::string DFUDevice::getSerialNumber() const {
    const std::string serial = irecv_util::serialFromClient(mClient);
    if (serial.empty()) {
        throw std::runtime_error("Failed to get serial number");
    }
    return serial;
}

std::string DFUDevice::getDeviceType() const {
    const std::string productType = irecv_util::productTypeFromClient(mClient);
    if (productType.empty()) {
        throw std::runtime_error("Failed to get device type");
    }
    return productType;
}

std::string DFUDevice::getFirmwareVersion() const {
    char* value = nullptr;
    if (irecv_getenv(mClient, "build-version", &value) == IRECV_E_SUCCESS && value != nullptr) {
        std::string result(value);
        free(value);
        return result;
    }
    return std::string();
}

std::vector<uint8_t> DFUDevice::readMemory(const uint64_t address, const uint64_t length) const {
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

void DFUDevice::writeMemory(const uint64_t address, const std::vector<uint8_t>& data) const {
    if (data.size() > 0xFFFFu) {
        throw std::runtime_error("Write length exceeds USB control transfer limit");
    }
    const int transferred = irecv_util::usbMemoryWrite(
        mClient, address, &data[0], static_cast<uint16_t>(data.size()));
    if (transferred < 0 || static_cast<size_t>(transferred) != data.size()) {
        throw std::runtime_error("Failed to write memory");
    }
}

void DFUDevice::sendCommand(const std::string& command) const {
    if (irecv_send_command(mClient, command.c_str()) != IRECV_E_SUCCESS) {
        throw std::runtime_error("Failed to send command");
    }
}

irecv_util::IRecvCommandResult DFUDevice::sendCommandWithResponse(const std::string& command,
                                                                  const uint64_t recvLength) const {
    return irecv_util::sendCommandWithResponse(mClient, command, recvLength);
}

uint32_t DFUDevice::getCommandReturn() const {
    uint32_t value = 0;
    if (!irecv_util::getCommandReturn(mClient, &value)) {
        throw std::runtime_error("Failed to read command return");
    }
    return value;
}

void DFUDevice::sendFile(const std::string& path, const unsigned int options) const {
    if (irecv_send_file(mClient, path.c_str(), options) != IRECV_E_SUCCESS) {
        throw std::runtime_error("Failed to send file: " + path);
    }
}

std::vector<uint8_t> DFUDevice::receiveResponse(const uint64_t length) const {
    std::vector<uint8_t> buffer(length);
    if (irecv_recv_buffer(mClient, reinterpret_cast<char*>(&buffer[0]), length) !=
        IRECV_E_SUCCESS) {
        throw std::runtime_error("Failed to receive response");
    }
    return buffer;
}

bool DFUDevice::isInDFUMode() const {
    return mClient != nullptr;
}

uint32_t DFUDevice::getCpid() const {
    return irecv_util::cpidFromClient(mClient);
}

uint64_t DFUDevice::getEcid() const {
    return irecv_util::ecidFromClient(mClient);
}

} /* namespace PP */
