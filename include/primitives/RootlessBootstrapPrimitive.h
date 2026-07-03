/*
 * RootlessBootstrapPrimitive.h
 *
 * Probe /var/jb layout on a jailbroken device via SSH (Normal mode).
 */

#ifndef PRIMITIVES_ROOTLESS_BOOTSTRAP_PRIMITIVE_H_
#define PRIMITIVES_ROOTLESS_BOOTSTRAP_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class RootlessBootstrapPrimitive : public Primitive {
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

#endif /* PRIMITIVES_ROOTLESS_BOOTSTRAP_PRIMITIVE_H_ */
