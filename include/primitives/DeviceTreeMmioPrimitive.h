/*
 * DeviceTreeMmioPrimitive.h
 *
 * Offline DeviceTree → MMIO catalog for rootless / PPL / phys R/W planning.
 */

#ifndef PRIMITIVES_DEVICE_TREE_MMIO_PRIMITIVE_H_
#define PRIMITIVES_DEVICE_TREE_MMIO_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class DeviceTreeMmioPrimitive : public Primitive {
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

#endif /* PRIMITIVES_DEVICE_TREE_MMIO_PRIMITIVE_H_ */
