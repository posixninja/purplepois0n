/*
 * IpswdHostProbePrimitive.h
 *
 * Host-side firmware analysis probe (ipswd / ipsw / in-tree parsers).
 */

#ifndef PRIMITIVES_IPSWD_HOST_PROBE_PRIMITIVE_H_
#define PRIMITIVES_IPSWD_HOST_PROBE_PRIMITIVE_H_

#include "CodesignPrimitive.h"

namespace PP {
namespace primitives {

class IpswdHostProbePrimitive : public CodesignPrimitive {
public:
    const char* name() const override;
    std::vector<PrimitiveOperation> supportedOperations() const override;
    std::vector<DeviceState> requiredDeviceStates() const override;
    bool canRun(const ExecutionContext& context) const override;
    PrimitiveResult execute(ExecutionContext& context) override;
    std::string targetKind() const override;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_IPSWD_HOST_PROBE_PRIMITIVE_H_ */
