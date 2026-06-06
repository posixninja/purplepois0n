/*
 * RamdiskShellPrimitive.h
 */

#ifndef PRIMITIVES_RAMDISK_SHELL_PRIMITIVE_H_
#define PRIMITIVES_RAMDISK_SHELL_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class RamdiskShellPrimitive : public Primitive {
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

#endif /* PRIMITIVES_RAMDISK_SHELL_PRIMITIVE_H_ */
