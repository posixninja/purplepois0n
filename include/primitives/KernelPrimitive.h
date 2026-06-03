/*
 * KernelPrimitive.h
 *
 * Abstract base for ring-0 memory / kext / slide-defeat primitives.
 */

#ifndef PRIMITIVES_KERNEL_PRIMITIVE_H_
#define PRIMITIVES_KERNEL_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class KernelPrimitive : public Primitive {
public:
    PrimitiveCategory category() const override final { return PrimitiveCategory::Kernel; }
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_KERNEL_PRIMITIVE_H_ */
