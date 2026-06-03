/*
 * DfuTransport.cpp
 */

#include "primitives/DfuTransport.h"
#include "DFUDevice.h"

namespace PP {
namespace primitives {

DfuTransport::DfuTransport(DFUDevice& device) : mDevice(device) {}

std::vector<uint8_t> DfuTransport::readMemory(uint64_t address, uint64_t length) {
    return mDevice.readMemory(address, length);
}

void DfuTransport::writeMemory(uint64_t address, const std::vector<uint8_t>& data) {
    mDevice.writeMemory(address, data);
}

bool DfuTransport::isLive() const {
    return true;
}

const char* DfuTransport::transportName() const {
    return "DfuTransport";
}

} /* namespace primitives */
} /* namespace PP */
