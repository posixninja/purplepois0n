/*
 * DpkgStorePrimitive.h
 *
 * Host dpkg store: build Packages index and sync to /var/jb over SSH.
 */

#ifndef PRIMITIVES_DPKG_STORE_PRIMITIVE_H_
#define PRIMITIVES_DPKG_STORE_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class DpkgStorePrimitive : public Primitive {
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

#endif /* PRIMITIVES_DPKG_STORE_PRIMITIVE_H_ */
