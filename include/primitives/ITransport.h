/*
 * ITransport.h
 *
 * Abstract byte transport for live USB or offline buffers.
 */

#ifndef PRIMITIVES_I_TRANSPORT_H_
#define PRIMITIVES_I_TRANSPORT_H_

#include <cstdint>
#include <vector>

namespace PP {
namespace primitives {

/**
 * @class ITransport
 * @brief Infrastructure for memory read/write — not an exploit category.
 */
class ITransport {
public:
    virtual ~ITransport() {}

    virtual std::vector<uint8_t> readMemory(uint64_t address, uint64_t length) = 0;
    virtual void writeMemory(uint64_t address, const std::vector<uint8_t>& data) = 0;

    /** @return true for live DFU/Recovery USB; false for in-memory buffers. */
    virtual bool isLive() const = 0;

    virtual const char* transportName() const = 0;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_I_TRANSPORT_H_ */
