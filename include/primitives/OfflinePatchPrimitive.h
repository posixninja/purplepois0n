/*
 * OfflinePatchPrimitive.h
 *
 * Offline codesign/kernel patch surface (probe-only; no bundled patterns).
 */

#ifndef PRIMITIVES_OFFLINE_PATCH_PRIMITIVE_H_
#define PRIMITIVES_OFFLINE_PATCH_PRIMITIVE_H_

#include "CodesignPrimitive.h"

namespace PP {
namespace primitives {

class OfflinePatchPrimitive : public CodesignPrimitive {
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

#endif /* PRIMITIVES_OFFLINE_PATCH_PRIMITIVE_H_ */
