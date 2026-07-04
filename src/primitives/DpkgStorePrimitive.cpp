/*
 * DpkgStorePrimitive.cpp
 */

#include "primitives/DpkgStorePrimitive.h"
#include "primitives/RootlessDelegate.h"
#include "primitives/RootlessLayout.h"
#include "store/DpkgStore.h"
#include "store/DpkgStoreSync.h"
#include "EnvUtil.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* DpkgStorePrimitive::name() const { return "dpkg-store"; }

PrimitiveCategory DpkgStorePrimitive::category() const {
    return PrimitiveCategory::Bootstrap;
}

std::vector<PrimitiveOperation> DpkgStorePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> DpkgStorePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Normal, DeviceState::Unknown};
}

bool DpkgStorePrimitive::canRun(const ExecutionContext& context) const {
    if (RootlessDelegate::preferRootless(context)) {
        return true;
    }
    const char* fixture = std::getenv("PURPLEPOIS0N_STORE_ROOT");
    return fixture != nullptr && fixture[0] != '\0';
}

PrimitiveResult DpkgStorePrimitive::execute(ExecutionContext& context) {
    const std::string storeRoot = store::resolveStoreRoot();
    Logger::info("  [Store] purplepois0n dpkg repo at " + storeRoot);

    std::string initError;
    if (!store::initRepository(storeRoot, &initError)) {
        Logger::error("  [Store] " + initError);
        return PrimitiveResult::Failed;
    }

    const store::StoreBuildResult built = store::buildRepository(storeRoot);
    if (!built.success) {
        Logger::error("  [Store] build failed: " + built.error);
        return PrimitiveResult::Failed;
    }

    Logger::info("  [Store] Packages: " + built.packagesPath + " (" +
                 std::to_string(built.packageCount) + " deb(s))");
    Logger::info("  [Store] device path: " + store::deviceStorePath());
    Logger::info("  [Store] apt source: " + store::aptSourceLine());

    if (!RootlessDelegate::sshConfigured(context)) {
        Logger::info("  [Store] offline — use --store-sync with --normal-ssh to push to device");
        return PrimitiveResult::Success;
    }

    const store::StoreSyncResult synced =
        store::syncStoreToDevice(RootlessDelegate::sshConnectOptions(context), storeRoot,
                                 context.allowMutation, store::StoreSyncMode::File);
    if (!synced.success) {
        Logger::error("  [Store] sync failed: " + synced.error);
        return PrimitiveResult::TransportError;
    }

    const char* installPkg = std::getenv("PURPLEPOIS0N_STORE_INSTALL");
    if (installPkg != nullptr && installPkg[0] != '\0') {
        std::string installError;
        if (!store::installPackageOnDevice(RootlessDelegate::sshConnectOptions(context),
                                           RootlessLayout::resolveJbroot(), installPkg,
                                           context.allowMutation, &installError)) {
            Logger::error("  [Store] install failed: " + installError);
            return PrimitiveResult::Failed;
        }
    }

    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
