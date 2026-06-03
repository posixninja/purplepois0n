/*
 * BufferTransport.cpp
 */

#include "primitives/BufferTransport.h"

#include <stdexcept>

namespace PP {
namespace primitives {

BufferTransport::BufferTransport(std::vector<uint8_t> buffer) : mBuffer(buffer) {}

std::vector<uint8_t> BufferTransport::readMemory(uint64_t address, uint64_t length) {
    if (address + length > mBuffer.size()) {
        throw std::runtime_error("BufferTransport read out of bounds");
    }
    return std::vector<uint8_t>(mBuffer.begin() + static_cast<size_t>(address),
                                mBuffer.begin() + static_cast<size_t>(address + length));
}

void BufferTransport::writeMemory(uint64_t address, const std::vector<uint8_t>& data) {
    if (address + data.size() > mBuffer.size()) {
        throw std::runtime_error("BufferTransport write out of bounds");
    }
    for (size_t i = 0; i < data.size(); ++i) {
        mBuffer[static_cast<size_t>(address + i)] = data[i];
    }
}

bool BufferTransport::isLive() const {
    return false;
}

const char* BufferTransport::transportName() const {
    return "BufferTransport";
}

} /* namespace primitives */
} /* namespace PP */
