/*
 * purplepois0n.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef PURPLEPOIS0N_H_
#define PURPLEPOIS0N_H_

#include "DeviceState.h"

// Forward declarations
namespace PP {
    class DeviceManager;
    class MobileDevice;
    class RecoveryDevice;
    class DFUDevice;
    class AFCService;

    namespace primitives {
        class ChainRunner;
        class PrimitiveRegistry;
    }
}

/**
 * @mainpage purplepois0n
 * 
 * @section intro_sec Introduction
 * 
 * purplepois0n is a jailbreak tool for iOS devices. It provides a framework
 * for interacting with iOS devices in various states (Normal, Recovery, DFU)
 * and can be extended with exploit code for jailbreaking.
 * 
 * @section usage_sec Usage
 * 
 * Basic usage:
 * @code
 * #include "purplepois0n.h"
 * 
 * int main() {
 *     PP::DeviceManager manager;
 *     PP::DeviceState state = manager.detectDeviceState();
 *     // ... use device based on state
 *     return 0;
 * }
 * @endcode
 * 
 * @section classes_sec Main Classes
 * 
 * - DeviceManager: Manages device detection and enumeration
 * - MobileDevice: Interface for devices in normal mode
 * - RecoveryDevice: Interface for devices in recovery mode
 * - DFUDevice: Interface for devices in DFU mode
 * - AFCService: File transfer service for normal mode devices
 *
 * Primitive framework (include/primitives/Primitives.h):
 * - ChainRunner: Staged probe/execute orchestration
 * - PrimitiveRegistry: Built-in primitive registration
 *
 * @see docs/LINEAGE.md Historical lineage (greenpois0n, absinthe) and jailbreak generations
 */

#endif /* PURPLEPOIS0N_H_ */

