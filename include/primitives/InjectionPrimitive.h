/*
 * InjectionPrimitive.h
 *
 * Abstract base for payload / tweak delivery primitives.
 */

#ifndef PRIMITIVES_INJECTION_PRIMITIVE_H_
#define PRIMITIVES_INJECTION_PRIMITIVE_H_

#include "Primitive.h"

#include <string>

namespace PP {
namespace primitives {

class InjectionPrimitive : public Primitive {
public:
    PrimitiveCategory category() const override final { return PrimitiveCategory::Injection; }

    /** Hint for delivery channel, e.g. "afc" or "backup". */
    virtual std::string deliveryPath() const = 0;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_INJECTION_PRIMITIVE_H_ */
