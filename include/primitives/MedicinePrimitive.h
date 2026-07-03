/*
 * MedicinePrimitive.h
 */

#ifndef PRIMITIVES_MEDICINE_PRIMITIVE_H_
#define PRIMITIVES_MEDICINE_PRIMITIVE_H_

#include "primitives/Primitive.h"

namespace PP {
namespace primitives {

class MedicinePrimitive : public Primitive {
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

#endif /* PRIMITIVES_MEDICINE_PRIMITIVE_H_ */
