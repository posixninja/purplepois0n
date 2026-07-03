/*
 * HostKernelPatchPrimitive.h
 *
 * Classic-chain Patchfind stage: host kernelcache analysis and user patch apply.
 */

#ifndef PRIMITIVES_HOST_KERNEL_PATCH_PRIMITIVE_H_
#define PRIMITIVES_HOST_KERNEL_PATCH_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class HostKernelPatchPrimitive : public Primitive {
public:
    const char* name() const override;
    PrimitiveCategory category() const override;
    std::vector<PrimitiveOperation> supportedOperations() const override;
    std::vector<DeviceState> requiredDeviceStates() const override;
    bool canRun(const ExecutionContext& context) const override;
    ChainStage gen6Stage() const override;
    PrimitiveResult execute(ExecutionContext& context) override;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_HOST_KERNEL_PATCH_PRIMITIVE_H_ */
