/*
 * BufferTransport.h
 *
 * In-memory ITransport for offline binary patch research.
 */

#ifndef PRIMITIVES_BUFFER_TRANSPORT_H_
#define PRIMITIVES_BUFFER_TRANSPORT_H_

#include "ITransport.h"

#include <vector>

namespace PP {
namespace primitives {

class BufferTransport : public ITransport {
public:
    explicit BufferTransport(std::vector<uint8_t> buffer);

    std::vector<uint8_t> readMemory(uint64_t address, uint64_t length) override;
    void writeMemory(uint64_t address, const std::vector<uint8_t>& data) override;
    bool isLive() const override;
    const char* transportName() const override;

    const std::vector<uint8_t>& buffer() const { return mBuffer; }
    std::vector<uint8_t>& buffer() { return mBuffer; }

private:
    std::vector<uint8_t> mBuffer;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_BUFFER_TRANSPORT_H_ */
