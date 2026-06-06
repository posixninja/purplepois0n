/*
 * RecoveryTransport.cpp
 */

#include "primitives/RecoveryTransport.h"
#include "RecoveryDevice.h"

namespace PP {
namespace primitives {

RecoveryTransport::RecoveryTransport(RecoveryDevice& device) : mDevice(device) {}

std::vector<uint8_t> RecoveryTransport::readMemory(uint64_t address, uint64_t length) {
    return mDevice.readMemory(address, length);
}

void RecoveryTransport::writeMemory(uint64_t address, const std::vector<uint8_t>& data) {
    mDevice.writeMemory(address, data);
}

bool RecoveryTransport::isLive() const {
    return true;
}

const char* RecoveryTransport::transportName() const {
    return "RecoveryTransport";
}

bool RecoveryTransport::sendFile(const std::string& path) {
    try {
        mDevice.sendFile(path);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RecoveryTransport::resetDevice() {
    try {
        mDevice.reset();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RecoveryTransport::rebootDevice() {
    try {
        mDevice.reboot();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RecoveryTransport::sendCommand(const std::string& command) {
    try {
        mDevice.sendCommand(command);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string RecoveryTransport::getDeviceEnv(const std::string& name) const {
    return mDevice.getEnv(name);
}

std::vector<uint8_t> RecoveryTransport::getApNonceBytes() const {
    return mDevice.getApNonce();
}

} /* namespace primitives */
} /* namespace PP */
