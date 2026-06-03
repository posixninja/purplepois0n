/*
 * DeviceManager.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef DEVICEMANAGER_H_
#define DEVICEMANAGER_H_

#include <memory>
#include <string>
#include <vector>
#include "../include/DeviceState.h"
#include "MobileDevice.h"
#include "RecoveryDevice.h"
#include "DFUDevice.h"

namespace PP {

struct DeviceInfo {
    std::string udid;
    uint64_t ecid = 0;
    uint32_t cpid = 0;
    DeviceState state = DeviceState::Unknown;
    std::string deviceType;
    std::string firmwareVersion;
};

/**
 * @class DeviceManager
 * @brief Manages device detection and enumeration across different device states
 */
class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    /**
     * @brief Detect the state of a connected device
     * @param udid Optional UDID to check, if empty checks first available device
     * @return DeviceState The detected state of the device
     */
    DeviceState detectDeviceState(const std::string& udid = "");

    /**
     * @brief Enumerate all connected devices
     * @return Vector of DeviceInfo structures for all connected devices
     */
    std::vector<DeviceInfo> enumerateDevices();

    /**
     * @brief Get a MobileDevice handle for a device in normal mode
     * @param udid Optional UDID, if empty uses first available device
     * @return Unique pointer to MobileDevice, or nullptr if not available
     */
    std::unique_ptr<MobileDevice> getMobileDevice(const std::string& udid = "");

    /**
     * @brief Get a RecoveryDevice handle for a device in recovery mode
     * @param ecid ECID of the device
     * @return Unique pointer to RecoveryDevice, or nullptr if not available
     */
    std::unique_ptr<RecoveryDevice> getRecoveryDevice(uint64_t ecid);

    /**
     * @brief Get a DFUDevice handle for a device in DFU mode
     * @return Unique pointer to DFUDevice, or nullptr if not available
     */
    std::unique_ptr<DFUDevice> getDFUDevice();

    /**
     * @brief ECID for a connected Recovery-mode device (0 if unknown).
     */
    uint64_t getRecoveryEcid();

    /**
     * @brief Check if any device is connected
     * @return True if at least one device is connected
     */
    bool hasConnectedDevice();

private:
    bool tryOpenNormalDevice(const std::string& udid = "");
    bool tryOpenRecoveryDevice();
    bool tryOpenDFUDevice();
};

} /* namespace PP */

#endif /* DEVICEMANAGER_H_ */

