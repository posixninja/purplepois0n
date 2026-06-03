/*
 * NormalModeProbePrimitive.h
 *
 * Normal-mode userland probe: installed app count via lockdown.
 */

#ifndef PRIMITIVES_NORMAL_MODE_PROBE_PRIMITIVE_H_
#define PRIMITIVES_NORMAL_MODE_PROBE_PRIMITIVE_H_

#include "InjectionPrimitive.h"

namespace PP {
namespace primitives {

class NormalModeProbePrimitive : public InjectionPrimitive {
public:
    const char* name() const override;
    std::vector<PrimitiveOperation> supportedOperations() const override;
    std::vector<DeviceState> requiredDeviceStates() const override;
    bool canRun(const ExecutionContext& context) const override;
    PrimitiveResult execute(ExecutionContext& context) override;
    std::string deliveryPath() const override;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_NORMAL_MODE_PROBE_PRIMITIVE_H_ */
