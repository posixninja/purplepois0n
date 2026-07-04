/*
 * PongoBootChainPrimitive.cpp
 *
 * Deprecated alias — usb-loader boot chain (PongoOS transport).
 */

#include "primitives/PongoBootChainPrimitive.h"
#include "primitives/UsbLoaderBootChainPrimitive.h"

namespace PP {
namespace primitives {

const char* PongoBootChainPrimitive::name() const { return "pongo-boot-chain"; }

PrimitiveCategory PongoBootChainPrimitive::category() const {
    return mDelegate.category();
}

std::vector<PrimitiveOperation> PongoBootChainPrimitive::supportedOperations() const {
    return mDelegate.supportedOperations();
}

std::vector<DeviceState> PongoBootChainPrimitive::requiredDeviceStates() const {
    return mDelegate.requiredDeviceStates();
}

bool PongoBootChainPrimitive::canRun(const ExecutionContext& context) const {
    return mDelegate.canRun(context);
}

PrimitiveResult PongoBootChainPrimitive::execute(ExecutionContext& context) {
    return mDelegate.execute(context);
}

} /* namespace primitives */
} /* namespace PP */
