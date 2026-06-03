/*
 * DfuTransport.h
 *
 * ITransport adapter wrapping DFUDevice USB memory I/O.
 */

#ifndef PRIMITIVES_DFU_TRANSPORT_H_
#define PRIMITIVES_DFU_TRANSPORT_H_

#include "ITransport.h"

namespace PP {
class DFUDevice;
}

namespace PP {
namespace primitives {

class DfuTransport : public ITransport {
public:
    explicit DfuTransport(DFUDevice& device);

    std::vector<uint8_t> readMemory(uint64_t address, uint64_t length) override;
    void writeMemory(uint64_t address, const std::vector<uint8_t>& data) override;
    bool isLive() const override;
    const char* transportName() const override;

    DFUDevice& device() { return mDevice; }
    const DFUDevice& device() const { return mDevice; }

private:
    DFUDevice& mDevice;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_DFU_TRANSPORT_H_ */
