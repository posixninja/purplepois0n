/*
 * UsbLoaderBootChainPrimitive.cpp
 *
 * Delivers an optional boot module + ramdisk over a USB loader bulk protocol.
 * PongoDevice implements the checkra1n/PongoOS dialect; the primitive itself is lane-generic.
 */

#include "primitives/UsbLoaderBootChainPrimitive.h"
#include "primitives/PongoDelegate.h"
#include "pongo/PongoDevice.h"
#include "RamdiskDelivery.h"
#include "EnvUtil.h"
#include "Logger.h"

#include <vector>

namespace PP {
namespace primitives {

namespace {

const char* kDefaultUsbLoaderBootArgs = "xargs serial=3 rootdev=md0";

bool usbLoaderBootRequested(const ExecutionContext& context) {
    if (context.bootDeliveryRun) {
        const BootDeliverySpec spec = resolveBootDelivery(context);
        return spec.lane == BootDeliveryLane::Auto ||
               spec.lane == BootDeliveryLane::UsbLoader;
    }
    if (context.pongoBootRun) {
        return true;
    }
    return PP::envFlagEnabled("PURPLEPOIS0N_PONGO_BOOT") ||
           PP::envFlagEnabled("PURPLEPOIS0N_BOOT_DELIVERY");
}

} /* anonymous */

const char* UsbLoaderBootChainPrimitive::name() const { return "usb-loader-boot-chain"; }

PrimitiveCategory UsbLoaderBootChainPrimitive::category() const {
    return PrimitiveCategory::Injection;
}

std::vector<PrimitiveOperation> UsbLoaderBootChainPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> UsbLoaderBootChainPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::DFU, DeviceState::Unknown};
}

bool UsbLoaderBootChainPrimitive::canRun(const ExecutionContext& context) const {
    return usbLoaderBootRequested(context);
}

PrimitiveResult UsbLoaderBootChainPrimitive::execute(ExecutionContext& context) {
    const BootDeliverySpec delivery = resolveBootDelivery(context);
    Logger::info("  [Boot] lane=" + std::string(bootDeliveryLaneLabel(delivery.lane)) +
                 " usb-loader: module + ramdisk → boot");

    if (delivery.lane != BootDeliveryLane::Auto &&
        delivery.lane != BootDeliveryLane::UsbLoader) {
        Logger::warn("  [Boot] usb-loader-boot-chain requires --boot-lane usb-loader (got " +
                     std::string(bootDeliveryLaneLabel(delivery.lane)) + ")");
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (!pongoLibusbAvailable()) {
        Logger::warn("  [Boot] libusb not linked — brew install libusb && rebuild");
        return PrimitiveResult::PrerequisitesMissing;
    }

    const std::string modulePath = resolveBootModulePath(context);
    if (modulePath.empty()) {
        Logger::warn("  [Boot] boot module required for usb-loader lane");
        Logger::warn("  [Boot] set --boot-module or PURPLEPOIS0N_BOOT_MODULE (make kpf)");
        return PrimitiveResult::PrerequisitesMissing;
    }
    Logger::info("  [Boot] module: " + modulePath);

    std::string ramdiskPath;
    if (!resolveRamdiskArtifactPath(context, &ramdiskPath)) {
        return PrimitiveResult::PrerequisitesMissing;
    }
    Logger::info("  [Boot] ramdisk: " + ramdiskPath);

    if (!context.allowMutation) {
        Logger::info("  [Boot] would upload module: " + modulePath);
        Logger::info("  [Boot] would upload ramdisk: " + ramdiskPath);
        Logger::info("  [Boot] would run loader boot sequence");
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [Boot] upload requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    if (context.pongoSpawnCheckra1n) {
        const PrimitiveResult spawn = PongoDelegate::spawnCheckra1nShell(true);
        if (spawn != PrimitiveResult::Success) {
            return spawn;
        }
    }

    if (!PongoDelegate::isPongoPresent()) {
        Logger::error("  [Boot] no USB loader device — run checkra1n -cp first");
        return PrimitiveResult::PrerequisitesMissing;
    }

    std::vector<uint8_t> moduleBytes;
    std::vector<uint8_t> ramdiskBytes;
    if (!readBootArtifactBytes(modulePath, &moduleBytes) ||
        !readBootArtifactBytes(ramdiskPath, &ramdiskBytes)) {
        return PrimitiveResult::Failed;
    }

    PongoDevice dev;
    if (!dev.open()) {
        return PrimitiveResult::TransportError;
    }

    std::string bootArgs = resolveBootArgsLine(context);
    if (bootArgs.empty()) {
        bootArgs = kDefaultUsbLoaderBootArgs;
    }
    if (!dev.bootCheckra1nSequence(moduleBytes, ramdiskBytes, bootArgs)) {
        return PrimitiveResult::Failed;
    }

    Logger::info("  [Boot] usb-loader boot sequence complete");
    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
