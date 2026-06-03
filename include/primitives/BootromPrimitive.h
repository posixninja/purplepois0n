/*
 * BootromPrimitive.h
 *
 * Abstract base for pre-iBoot / bootrom-stage primitives.
 */

#ifndef PRIMITIVES_BOOTROM_PRIMITIVE_H_
#define PRIMITIVES_BOOTROM_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class BootromPrimitive : public Primitive {
public:
    PrimitiveCategory category() const override final { return PrimitiveCategory::Bootrom; }

    /** Bootrom primitives require DFU mode when operating on a live device. */
    virtual bool requiresDfu() const { return true; }
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_BOOTROM_PRIMITIVE_H_ */
