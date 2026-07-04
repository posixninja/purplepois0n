/*
 * UsbLoaderBootChainPrimitive.h
 *
 * USB secondary-loader boot chain (PongoOS protocol today; transport-specific).
 * Ramdisk artifact and boot module are independent inputs.
 */

#ifndef PRIMITIVES_USB_LOADER_BOOT_CHAIN_PRIMITIVE_H_
#define PRIMITIVES_USB_LOADER_BOOT_CHAIN_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class UsbLoaderBootChainPrimitive : public Primitive {
public:
    const char* name() const override;
    PrimitiveCategory category() const override;
    std::vector<PrimitiveOperation> supportedOperations() const override;
    std::vector<DeviceState> requiredDeviceStates() const override;
    bool canRun(const ExecutionContext& context) const override;
    PrimitiveResult execute(ExecutionContext& context) override;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_USB_LOADER_BOOT_CHAIN_PRIMITIVE_H_ */
