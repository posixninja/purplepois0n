/*
 * PongoDevice.cpp
 */

#include "pongo/PongoDevice.h"
#include "Logger.h"

#include <vector>

#ifdef PURPLEPOIS0N_HAVE_LIBUSB
#include <libusb-1.0/libusb.h>
#endif

namespace PP {

bool pongoLibusbAvailable() {
#ifdef PURPLEPOIS0N_HAVE_LIBUSB
    return true;
#else
    return false;
#endif
}

#ifdef PURPLEPOIS0N_HAVE_LIBUSB

struct PongoDevice::Impl {
    libusb_context* ctx = nullptr;
    libusb_device_handle* handle = nullptr;
};

namespace {

constexpr uint8_t kReqTypeOut = 0x21;
constexpr uint8_t kReqTypeIn = 0xa1;

bool initLibusb(libusb_context** ctx) {
    return libusb_init(ctx) == 0;
}

libusb_device_handle* openPongoHandle(libusb_context* ctx) {
    return libusb_open_device_with_vid_pid(ctx, PongoDevice::kUsbVendor, PongoDevice::kUsbProduct);
}

} /* anonymous namespace */

PongoDevice::PongoDevice() : mImpl(new Impl()) {}

PongoDevice::~PongoDevice() { close(); delete mImpl; }

bool PongoDevice::isPresent() {
    libusb_context* ctx = nullptr;
    if (!initLibusb(&ctx)) {
        return false;
    }
    libusb_device_handle* handle = openPongoHandle(ctx);
    const bool found = handle != nullptr;
    if (handle != nullptr) {
        libusb_close(handle);
    }
    libusb_exit(ctx);
    return found;
}

bool PongoDevice::open() {
    if (mImpl->handle != nullptr) {
        return true;
    }
    if (!initLibusb(&mImpl->ctx)) {
        Logger::error("  [Pongo] libusb_init failed");
        return false;
    }
    mImpl->handle = openPongoHandle(mImpl->ctx);
    if (mImpl->handle == nullptr) {
        Logger::warn("  [Pongo] no device 05ac:4141 (PongoOS shell not reachable)");
        libusb_exit(mImpl->ctx);
        mImpl->ctx = nullptr;
        return false;
    }
    if (libusb_kernel_driver_active(mImpl->handle, 0) == 1) {
        libusb_detach_kernel_driver(mImpl->handle, 0);
    }
    const int cfg = libusb_set_configuration(mImpl->handle, 1);
    if (cfg != 0 && cfg != LIBUSB_ERROR_BUSY) {
        Logger::warn("  [Pongo] set_configuration: " + std::to_string(cfg));
    }
    if (libusb_claim_interface(mImpl->handle, 0) != 0) {
        Logger::error("  [Pongo] claim_interface failed");
        libusb_close(mImpl->handle);
        mImpl->handle = nullptr;
        libusb_exit(mImpl->ctx);
        mImpl->ctx = nullptr;
        return false;
    }
    return true;
}

void PongoDevice::close() {
    if (mImpl->handle != nullptr) {
        libusb_release_interface(mImpl->handle, 0);
        libusb_close(mImpl->handle);
        mImpl->handle = nullptr;
    }
    if (mImpl->ctx != nullptr) {
        libusb_exit(mImpl->ctx);
        mImpl->ctx = nullptr;
    }
}

bool PongoDevice::isOpen() const { return mImpl->handle != nullptr; }

bool PongoDevice::ctrlOut(uint8_t request, uint16_t value, const uint8_t* data, uint16_t length) {
    if (mImpl->handle == nullptr) {
        return false;
    }
    const int transferred =
        libusb_control_transfer(mImpl->handle, kReqTypeOut, request, value, 0, const_cast<uint8_t*>(data),
                                length, 5000);
    if (transferred < 0) {
        Logger::error("  [Pongo] control OUT failed req=" + std::to_string(request));
        return false;
    }
    if (data != nullptr && static_cast<int>(length) != transferred) {
        Logger::warn("  [Pongo] short control OUT write");
    }
    return true;
}

bool PongoDevice::ctrlIn(uint8_t request, uint16_t value, uint8_t* data, uint16_t length,
                         int* transferred) {
    if (mImpl->handle == nullptr) {
        return false;
    }
    *transferred =
        libusb_control_transfer(mImpl->handle, kReqTypeIn, request, value, 0, data, length, 5000);
    if (*transferred < 0) {
        Logger::error("  [Pongo] control IN failed req=" + std::to_string(request));
        return false;
    }
    return true;
}

bool PongoDevice::sendCommand(const std::string& cmd) {
    const std::string line = cmd + "\n";
    return ctrlOut(3, 0, reinterpret_cast<const uint8_t*>(line.data()),
                   static_cast<uint16_t>(line.size()));
}

bool PongoDevice::getStdOut(std::string* out) {
    if (out == nullptr) {
        return false;
    }
    out->clear();
    uint8_t progress[1] = {1};
    uint8_t buffer[0x1000];

    while (progress[0] == 1) {
        int n = 0;
        if (!ctrlIn(2, 0, progress, 1, &n) || n == 0) {
            return false;
        }
        if (progress[0] != 1) {
            break;
        }
        if (!ctrlIn(1, 0, buffer, sizeof(buffer), &n)) {
            return false;
        }
        if (n == 0) {
            break;
        }
        out->append(reinterpret_cast<char*>(buffer), static_cast<size_t>(n));
    }
    return true;
}

bool PongoDevice::finishCommandPhase() {
    if (!ctrlOut(2, 0, nullptr, 0)) {
        return false;
    }
    return ctrlOut(1, 0, nullptr, 0);
}

bool PongoDevice::bulkUpload(const std::vector<uint8_t>& data, unsigned int timeoutMs) {
    if (mImpl->handle == nullptr) {
        return false;
    }
    int transferred = 0;
    if (data.empty()) {
        const int rc =
            libusb_bulk_transfer(mImpl->handle, kBulkOutEndpoint, nullptr, 0, &transferred, timeoutMs);
        return rc == 0;
    }
    const int rc = libusb_bulk_transfer(mImpl->handle, kBulkOutEndpoint,
                                        const_cast<uint8_t*>(data.data()),
                                        static_cast<int>(data.size()), &transferred, timeoutMs);
    if (rc != 0) {
        Logger::error("  [Pongo] bulk upload failed: " + std::to_string(rc));
        return false;
    }
    if (transferred != static_cast<int>(data.size())) {
        Logger::warn("  [Pongo] partial bulk upload");
    }
    return true;
}

bool PongoDevice::bootCheckra1nSequence(const std::vector<uint8_t>& kpf,
                                        const std::vector<uint8_t>& rdsk,
                                        const std::string& xargsLine) {
    if (kpf.empty() || rdsk.empty()) {
        Logger::error("  [Pongo] KPF and ramdisk DMG required");
        return false;
    }

    auto sendShell = [this](const std::string& cmd) -> bool {
        if (!ctrlOut(4, 0, nullptr, 0)) {
            return false;
        }
        if (!sendCommand(cmd)) {
            return false;
        }
        return finishCommandPhase();
    };

    if (!sendShell(xargsLine)) {
        return false;
    }
    if (!bulkUpload(kpf)) {
        return false;
    }
    if (!sendShell("modload")) {
        return false;
    }
    if (!bulkUpload(rdsk)) {
        return false;
    }
    const std::vector<uint8_t> empty;
    if (!bulkUpload(empty)) {
        return false;
    }
    if (!ctrlOut(4, 0, nullptr, 0)) {
        return false;
    }
    if (!sendCommand("ramdisk")) {
        return false;
    }
    if (!ctrlOut(4, 0, nullptr, 0)) {
        return false;
    }
    if (!sendCommand("kpf_flags 1")) {
        return false;
    }
    if (!ctrlOut(4, 0, nullptr, 0)) {
        return false;
    }
    if (!sendCommand("bootx")) {
        return false;
    }
    Logger::info("  [Pongo] bootx sent — USB disconnect is expected");
    return true;
}

#else /* !PURPLEPOIS0N_HAVE_LIBUSB */

struct PongoDevice::Impl {};

PongoDevice::PongoDevice() : mImpl(new Impl()) {}
PongoDevice::~PongoDevice() { delete mImpl; }

bool PongoDevice::isPresent() { return false; }
bool PongoDevice::open() { return false; }
void PongoDevice::close() {}
bool PongoDevice::isOpen() const { return false; }
bool PongoDevice::sendCommand(const std::string&) { return false; }
bool PongoDevice::getStdOut(std::string*) { return false; }
bool PongoDevice::bulkUpload(const std::vector<uint8_t>&, unsigned int) { return false; }
bool PongoDevice::bootCheckra1nSequence(const std::vector<uint8_t>&, const std::vector<uint8_t>&,
                                        const std::string&) {
    return false;
}

#endif /* PURPLEPOIS0N_HAVE_LIBUSB */

} /* namespace PP */
