/*
 * Checkm8BootromPrimitive.h
 *
 * Bootrom probe/execute primitive delegating to Checkm8.
 */

#ifndef PRIMITIVES_CHECKM8_BOOTROM_PRIMITIVE_H_
#define PRIMITIVES_CHECKM8_BOOTROM_PRIMITIVE_H_

#include "BootromPrimitive.h"
#include "Checkm8.h"

namespace PP {
namespace primitives {

class Checkm8BootromPrimitive : public BootromPrimitive {
public:
    const char* name() const override;
    std::vector<PrimitiveOperation> supportedOperations() const override;
    std::vector<DeviceState> requiredDeviceStates() const override;
    bool canRun(const ExecutionContext& context) const override;
    PrimitiveResult execute(ExecutionContext& context) override;

private:
    Checkm8DeviceInfo mLastProbe;
    bool mHasProbe = false;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_CHECKM8_BOOTROM_PRIMITIVE_H_ */
