/*
 * RecoveryUploadPrimitive.h
 *
 * Phase 7.5: signed Recovery component upload (iBSS/iBEC) after TSS personalize.
 */

#ifndef PRIMITIVES_RECOVERY_UPLOAD_PRIMITIVE_H_
#define PRIMITIVES_RECOVERY_UPLOAD_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class RecoveryUploadPrimitive : public Primitive {
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

#endif /* PRIMITIVES_RECOVERY_UPLOAD_PRIMITIVE_H_ */
