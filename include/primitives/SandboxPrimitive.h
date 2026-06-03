/*
 * SandboxPrimitive.h
 *
 * Abstract base for sandbox profile / container escape primitives.
 */

#ifndef PRIMITIVES_SANDBOX_PRIMITIVE_H_
#define PRIMITIVES_SANDBOX_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class SandboxPrimitive : public Primitive {
public:
    PrimitiveCategory category() const override final { return PrimitiveCategory::Sandbox; }
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_SANDBOX_PRIMITIVE_H_ */
