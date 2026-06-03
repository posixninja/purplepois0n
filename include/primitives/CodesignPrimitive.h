/*
 * CodesignPrimitive.h
 *
 * Abstract base for AMFI / code-signing enforcement bypass primitives.
 */

#ifndef PRIMITIVES_CODESIGN_PRIMITIVE_H_
#define PRIMITIVES_CODESIGN_PRIMITIVE_H_

#include "Primitive.h"

#include <string>

namespace PP {
namespace primitives {

class CodesignPrimitive : public Primitive {
public:
    PrimitiveCategory category() const override final { return PrimitiveCategory::Codesign; }

    /** Target binary kind, e.g. "kernelcache" or "amfi". */
    virtual std::string targetKind() const = 0;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_CODESIGN_PRIMITIVE_H_ */
