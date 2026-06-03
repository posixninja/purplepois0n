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

} /* namespace primitives */
} /* namespace PP */
