/*
 * RecoveryDevice.cpp
 */

#include "RecoveryDevice.h"

#include <cstdlib>
#include <vector>

namespace PP {

RecoveryDevice::RecoveryDevice(const uint64_t ecid, IRecvProgressCallback progress)
    : mECID(ecid), mClient(getClient()) {
    if (progress) {
        m_progress.reset(new IRecvProgressSubscription(mClient, progress));
    }
}

void RecoveryDevice::sendFile(const std::string& path, const unsigned int options) const {
    if (irecv_send_file(mClient, path.c_str(), options) != IRECV_E_SUCCESS) {
        throw std::runtime_error("Failed to send file: " + path);
    }
}

void RecoveryDevice::reset() const {
    if (irecv_reset(mClient) != IRECV_E_SUCCESS) {
        throw std::runtime_error("irecv_reset failed");
    }
}

void RecoveryDevice::reboot() const {
    if (irecv_reboot(mClient) != IRECV_E_SUCCESS) {
        throw std::runtime_error("irecv_reboot failed");
    }
}

std::string RecoveryDevice::getEnv(const std::string& name) const {
    char* value = nullptr;
    if (irecv_getenv(mClient, name.c_str(), &value) == IRECV_E_SUCCESS && value != nullptr) {
        std::string result(value);
        free(value);
        return result;
    }
    return std::string();
}

uint32_t RecoveryDevice::getCpid() const {
    return irecv_util::cpidFromClient(mClient);
}

uint32_t RecoveryDevice::getBoardId() const {
    const struct irecv_device_info* info = irecv_get_device_info(mClient);
    if (info != nullptr && info->have_bdid) {
        return info->bdid;
    }
    return 0;
}

std::vector<uint8_t> RecoveryDevice::getApNonce() const {
    const struct irecv_device_info* info = irecv_get_device_info(mClient);
    if (info != nullptr && info->ap_nonce != nullptr && info->ap_nonce_size > 0) {
        return std::vector<uint8_t>(info->ap_nonce, info->ap_nonce + info->ap_nonce_size);
    }
    const std::string hex = getEnv("NONCE");
    std::vector<uint8_t> bytes;
    if (hex.size() >= 2 && (hex.size() % 2) == 0) {
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            const char pair[3] = {hex[i], hex[i + 1], '\0'};
            bytes.push_back(static_cast<uint8_t>(strtoul(pair, nullptr, 16)));
        }
    }
    return bytes;
}

} /* namespace PP */
