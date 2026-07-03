/*
 * AnthraxPayload.h
 *
 * Modernized anthrax: SSH-able ramdisk with live system access.
 * Boots after kernel with ability to mount main system and execute patches.
 * Complements cyanide (iBoot debugger) for complete bootchain debugging.
 */

#ifndef ANTHRAX_PAYLOAD_H_
#define ANTHRAX_PAYLOAD_H_

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "primitives/PrimitiveTypes.h"

namespace PP {

using primitives::ExecutionContext;

class DeviceManager;

/**
 * @class AnthraxPayload
 * @brief SSH-able ramdisk environment for post-kernel access.
 *        Can mount main system and execute patches directly.
 */
class AnthraxPayload {
public:
    AnthraxPayload();
    ~AnthraxPayload();

    const char* name() const { return "anthrax"; }

    // Ramdisk setup and initialization
    bool buildRamdisk(const ExecutionContext& context);
    bool injectIntoBootChain(DeviceManager& manager, const ExecutionContext& context);

    // SSH connectivity
    bool waitForSSH(int timeoutSeconds = 60);
    bool executeRemote(const std::string& command, std::string& output);

    // Main system mounting and patching
    bool mountMainSystem();
    bool unmountMainSystem();
    bool isMainSystemMounted() const { return mMainSystemMounted; }

    // Live patching through ramdisk
    bool applyLivePatches(const ExecutionContext& context);

private:
    bool mInitialized;
    bool mMainSystemMounted;
    std::string mRamdiskPath;
    std::string mDeviceIP;
    uint16_t mSSHPort;

    // Ramdisk building
    bool buildBaseRamdisk(const ExecutionContext& context);
    bool injectSSHTools();
    bool injectPatchers();
    bool compressRamdisk();
};

} /* namespace PP */

#endif /* ANTHRAX_PAYLOAD_H_ */
