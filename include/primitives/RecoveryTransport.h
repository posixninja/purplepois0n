/*
 * RecoveryTransport.h
 *
 * ITransport adapter wrapping RecoveryDevice USB memory I/O.
 */

#ifndef PRIMITIVES_RECOVERY_TRANSPORT_H_
#define PRIMITIVES_RECOVERY_TRANSPORT_H_

#include "ITransport.h"

namespace PP {
class RecoveryDevice;
}

namespace PP {
namespace primitives {

class RecoveryTransport : public ITransport {
public:
    explicit RecoveryTransport(RecoveryDevice& device);

    std::vector<uint8_t> readMemory(uint64_t address, uint64_t length) override;
    void writeMemory(uint64_t address, const std::vector<uint8_t>& data) override;
    bool isLive() const override;
    const char* transportName() const override;
    bool sendFile(const std::string& path) override;
    bool resetDevice() override;
    bool rebootDevice() override;
    bool sendCommand(const std::string& command) override;
    std::string getDeviceEnv(const std::string& name) const override;
    std::vector<uint8_t> getApNonceBytes() const override;

    RecoveryDevice& device() { return mDevice; }
    const RecoveryDevice& device() const { return mDevice; }

private:
    RecoveryDevice& mDevice;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_RECOVERY_TRANSPORT_H_ */
