/*
 * MobileBackup2ProbePrimitive.cpp
 */

#include "primitives/MobileBackup2ProbePrimitive.h"
#include "Logger.h"

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/mobilebackup2.h>

namespace PP {
namespace primitives {

const char* MobileBackup2ProbePrimitive::name() const { return "mobilebackup2-probe"; }

PrimitiveCategory MobileBackup2ProbePrimitive::category() const {
    return PrimitiveCategory::Sandbox;
}

std::vector<PrimitiveOperation> MobileBackup2ProbePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe};
}

std::vector<DeviceState> MobileBackup2ProbePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Normal};
}

bool MobileBackup2ProbePrimitive::canRun(const ExecutionContext& context) const {
    return context.deviceState == DeviceState::Normal && !context.udid.empty();
}

PrimitiveResult MobileBackup2ProbePrimitive::execute(ExecutionContext& context) {
    Logger::info("  [MB2]    probing com.apple.mobilebackup2 (connect + version only)");

    idevice_t device = nullptr;
    if (idevice_new_with_options(&device, context.udid.c_str(), IDEVICE_LOOKUP_USBMUX) !=
        IDEVICE_E_SUCCESS) {
        Logger::warn("  [MB2]    idevice connect failed");
        return PrimitiveResult::PrerequisitesMissing;
    }

    mobilebackup2_client_t client = nullptr;
    const mobilebackup2_error_t startErr =
        mobilebackup2_client_start_service(device, &client, "purplepois0n");
    if (startErr != MOBILEBACKUP2_E_SUCCESS || client == nullptr) {
        Logger::warn("  [MB2]    mobilebackup2_client_start_service failed (" +
                     std::to_string(startErr) + ")");
        idevice_free(device);
        return PrimitiveResult::Failed;
    }

    double localVersions[] = {2.0, 2.1, 3.0};
    double remoteVersion = 0.0;
    const mobilebackup2_error_t versionErr = mobilebackup2_version_exchange(
        client, localVersions, static_cast<char>(3), &remoteVersion);

    if (versionErr == MOBILEBACKUP2_E_SUCCESS) {
        Logger::info("  [MB2]    negotiated protocol version: " +
                     std::to_string(remoteVersion));
    } else {
        Logger::warn("  [MB2]    version exchange failed (" + std::to_string(versionErr) + ")");
    }

    Logger::info("  [MB2]    restore/staging — NOT in-tree (absinthe-era boundary)");

    mobilebackup2_client_free(client);
    idevice_free(device);

    return (versionErr == MOBILEBACKUP2_E_SUCCESS) ? PrimitiveResult::Success
                                                   : PrimitiveResult::Failed;
}

} /* namespace primitives */
} /* namespace PP */
