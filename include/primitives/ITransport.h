/*
 * ITransport.h
 *
 * Abstract byte transport for live USB or offline buffers.
 */

#ifndef PRIMITIVES_I_TRANSPORT_H_
#define PRIMITIVES_I_TRANSPORT_H_

#include <cstdint>
#include <string>
#include <vector>
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

    /** Upload signed firmware component (Recovery iBSS/iBEC); default unsupported. */
    virtual bool sendFile(const std::string& path) {
        (void)path;
        return false;
    }

    /** irecv_reset after upload (Recovery only). */
    virtual bool resetDevice() { return false; }

    /** irecv_reboot after upload (Recovery only). */
    virtual bool rebootDevice() { return false; }

    /** iBoot command (e.g. go) in Recovery; default unsupported. */
    virtual bool sendCommand(const std::string& command) {
        (void)command;
        return false;
    }

    /** iBoot env in Recovery (e.g. NONCE); empty when unsupported. */
    virtual std::string getDeviceEnv(const std::string& name) const {
        (void)name;
        return std::string();
    }

    /** Binary ApNonce from irecv device info when available. */
    virtual std::vector<uint8_t> getApNonceBytes() const { return std::vector<uint8_t>(); }
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_I_TRANSPORT_H_ */
