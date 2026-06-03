/*
 * DeviceManager.cpp
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#include "DeviceManager.h"
#include "Checkm8.h"
#include "IRecvUtil.h"
#include <libimobiledevice/libimobiledevice.h>
#include <libirecovery.h>
#include <stdexcept>

namespace {

void fillIrecvDeviceInfo(irecv_client_t client, PP::DeviceInfo& info) {
    info.ecid = PP::irecv_util::ecidFromClient(client);
    info.cpid = PP::irecv_util::cpidFromClient(client);

    const std::string productType = PP::irecv_util::productTypeFromClient(client);
    if (!productType.empty()) {
        info.deviceType = productType;
    }

    char* firmwareVersion = nullptr;
    if (irecv_getenv(client, "build-version", &firmwareVersion) == IRECV_E_SUCCESS &&
        firmwareVersion != nullptr) {
        info.firmwareVersion = firmwareVersion;
        free(firmwareVersion);
    }
}

bool tryOpenIrecvClient(irecv_client_t* client) {
    return PP::irecv_util::openWithEcidRetry(client, 0) == IRECV_E_SUCCESS && *client != nullptr;
}

} /* anonymous namespace */

namespace PP {

DeviceManager::DeviceManager() {
}

DeviceManager::~DeviceManager() {
}

DeviceState DeviceManager::detectDeviceState(const std::string& udid) {
    if (tryOpenDFUDevice()) {
        return DeviceState::DFU;
    }

    if (tryOpenRecoveryDevice()) {
        return DeviceState::Recovery;
    }

    if (tryOpenNormalDevice(udid)) {
        return DeviceState::Normal;
    }

    return DeviceState::Unknown;
}

std::vector<DeviceInfo> DeviceManager::enumerateDevices() {
    std::vector<DeviceInfo> devices;

    char** udids = nullptr;
    int count = 0;
    if (idevice_get_device_list(&udids, &count) == IDEVICE_E_SUCCESS && count > 0) {
        for (int i = 0; i < count; i++) {
            DeviceInfo info;
            info.udid = udids[i];
            info.state = DeviceState::Normal;

            try {
                MobileDevice device(udids[i]);
                info.deviceType = device.getDeviceType();
                info.firmwareVersion = device.getValue("", "ProductVersion");
            } catch (...) {
            }

            devices.push_back(info);
        }
        idevice_device_list_free(udids);
    }

    irecv_client_t recoveryClient = nullptr;
    if (tryOpenIrecvClient(&recoveryClient)) {
        int mode = IRECV_K_RECOVERY_MODE_1;
        bool isRecovery = true;
        if (irecv_get_mode(recoveryClient, &mode) == IRECV_E_SUCCESS) {
            isRecovery = irecv_util::isRecoveryMode(mode);
        }

        if (isRecovery) {
            DeviceInfo info;
            info.state = DeviceState::Recovery;
            fillIrecvDeviceInfo(recoveryClient, info);
            devices.push_back(info);
        }
        irecv_close(recoveryClient);
    }

    irecv_client_t dfuClient = nullptr;
    if (tryOpenIrecvClient(&dfuClient)) {
        int mode = IRECV_K_RECOVERY_MODE_1;
        if (irecv_get_mode(dfuClient, &mode) == IRECV_E_SUCCESS && irecv_util::isDfuMode(mode)) {
            DeviceInfo info;
            info.state = DeviceState::DFU;
            fillIrecvDeviceInfo(dfuClient, info);
            devices.push_back(info);
        }
        irecv_close(dfuClient);
    }

    return devices;
}

std::unique_ptr<MobileDevice> DeviceManager::getMobileDevice(const std::string& udid) {
    try {
        if (udid.empty()) {
            return std::make_unique<MobileDevice>();
        }
        return std::make_unique<MobileDevice>(udid);
    } catch (const std::exception&) {
        return nullptr;
    }
}

std::unique_ptr<RecoveryDevice> DeviceManager::getRecoveryDevice(uint64_t ecid) {
    try {
        return std::make_unique<RecoveryDevice>(ecid);
    } catch (const std::exception&) {
        return nullptr;
    }
}

std::unique_ptr<DFUDevice> DeviceManager::getDFUDevice() {
    try {
        return std::make_unique<DFUDevice>();
    } catch (const std::exception&) {
        return nullptr;
    }
}

bool DeviceManager::hasConnectedDevice() {
    return detectDeviceState() != DeviceState::Unknown;
}

uint64_t DeviceManager::getRecoveryEcid() {
    const std::vector<DeviceInfo> devices = enumerateDevices();
    for (size_t i = 0; i < devices.size(); ++i) {
        if (devices[i].state == DeviceState::Recovery && devices[i].ecid != 0) {
            return devices[i].ecid;
        }
    }
    return irecv_util::probeRecoveryEcid();
}

bool DeviceManager::tryOpenNormalDevice(const std::string& udid) {
    idevice_t device = nullptr;
    idevice_error_t error;

    if (udid.empty()) {
        error = idevice_new(&device, NULL);
    } else {
        error = idevice_new_with_options(&device, udid.c_str(), IDEVICE_LOOKUP_USBMUX);
    }

    if (error == IDEVICE_E_SUCCESS && device != nullptr) {
        idevice_free(device);
        return true;
    }

    return false;
}

bool DeviceManager::tryOpenRecoveryDevice() {
    irecv_client_t client = nullptr;
    if (!tryOpenIrecvClient(&client)) {
        return false;
    }

    int mode = IRECV_K_RECOVERY_MODE_1;
    bool isRecovery = true;
    if (irecv_get_mode(client, &mode) == IRECV_E_SUCCESS) {
        isRecovery = irecv_util::isRecoveryMode(mode);
    }

    irecv_close(client);
    return isRecovery;
}

bool DeviceManager::tryOpenDFUDevice() {
    irecv_client_t client = nullptr;
    if (!tryOpenIrecvClient(&client)) {
        return false;
    }

    int mode = IRECV_K_RECOVERY_MODE_1;
    bool isDFU = false;
    if (irecv_get_mode(client, &mode) == IRECV_E_SUCCESS) {
        isDFU = irecv_util::isDfuMode(mode);
    }

    irecv_close(client);
    return isDFU;
}

} /* namespace PP */
