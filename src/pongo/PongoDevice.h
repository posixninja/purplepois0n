/*
 * PongoDevice.h
 *
 * USB client for PongoOS (checkra1n secondary bootloader).
 * VID 0x05ac / PID 0x4141 — mirrors external/ipsw/pkg/usb/pongo/pongo.go.
 */

#ifndef PONGO_DEVICE_H_
#define PONGO_DEVICE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace PP {

/** Returns true when built with libusb (PURPLEPOIS0N_HAVE_LIBUSB). */
bool pongoLibusbAvailable();

class PongoDevice {
public:
    static constexpr uint16_t kUsbVendor = 0x05ac;
    static constexpr uint16_t kUsbProduct = 0x4141;
    static constexpr uint8_t kBulkOutEndpoint = 0x02;

    PongoDevice();
    ~PongoDevice();

    PongoDevice(const PongoDevice&) = delete;
    PongoDevice& operator=(const PongoDevice&) = delete;

    static bool isPresent();

    bool open();
    void close();
    bool isOpen() const;

    bool sendCommand(const std::string& cmd);
    bool getStdOut(std::string* out);
    bool bulkUpload(const std::vector<uint8_t>& data, unsigned int timeoutMs = 120000);

    bool bootCheckra1nSequence(const std::vector<uint8_t>& kpf, const std::vector<uint8_t>& rdsk,
                               const std::string& xargsLine = "xargs serial=3 rootdev=md0");

private:
    bool ctrlOut(uint8_t request, uint16_t value, const uint8_t* data, uint16_t length);
    bool ctrlIn(uint8_t request, uint16_t value, uint8_t* data, uint16_t length, int* transferred);
    bool finishCommandPhase();

    struct Impl;
    Impl* mImpl;
};

} /* namespace PP */

#endif /* PONGO_DEVICE_H_ */
