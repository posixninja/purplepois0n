/*
 * AfcInjectionPrimitive.h
 *
 * Normal-mode AFC injection probe (no default payload delivery).
 */

#ifndef PRIMITIVES_AFC_INJECTION_PRIMITIVE_H_
#define PRIMITIVES_AFC_INJECTION_PRIMITIVE_H_

#include "InjectionPrimitive.h"

namespace PP {
namespace primitives {

class AfcInjectionPrimitive : public InjectionPrimitive {
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

#endif /* PRIMITIVES_AFC_INJECTION_PRIMITIVE_H_ */
