/*
 * Primitive.cpp
 */

#include "primitives/Primitive.h"

namespace PP {
namespace primitives {

bool Primitive::supports(PrimitiveOperation op) const {
    return supportsOperation(supportedOperations(), op);
}

} /* namespace primitives */
} /* namespace PP */
