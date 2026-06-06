/*
 * SandboxCapabilityProbePrimitive.h
 *
 * Sandbox / backup-staging boundary probe (parse-only in-tree).
 */

#ifndef PRIMITIVES_SANDBOX_CAPABILITY_PROBE_PRIMITIVE_H_
#define PRIMITIVES_SANDBOX_CAPABILITY_PROBE_PRIMITIVE_H_

#include "SandboxPrimitive.h"

namespace PP {
namespace primitives {

class SandboxCapabilityProbePrimitive : public SandboxPrimitive {
public:
    const char* name() const override;
    std::vector<PrimitiveOperation> supportedOperations() const override;
    std::vector<DeviceState> requiredDeviceStates() const override;
    bool canRun(const ExecutionContext& context) const override;
    PrimitiveResult execute(ExecutionContext& context) override;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_SANDBOX_CAPABILITY_PROBE_PRIMITIVE_H_ */
