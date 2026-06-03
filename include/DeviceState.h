/*
 * DeviceState.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef DEVICESTATE_H_
#define DEVICESTATE_H_

namespace PP {

/**
 * @enum DeviceState
 * @brief Represents the current connection state of an iOS device
 */
enum class DeviceState {
    Unknown,    ///< Device state could not be determined
    Normal,     ///< Device is in normal mode (booted iOS)
    Recovery,   ///< Device is in recovery mode
    DFU         ///< Device is in DFU (Device Firmware Update) mode
};

} /* namespace PP */

#endif /* DEVICESTATE_H_ */

