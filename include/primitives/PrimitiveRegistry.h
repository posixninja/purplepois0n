/*
 * PrimitiveRegistry.h
 *
 * Registration and lookup for built-in and plugin primitives.
 */

#ifndef PRIMITIVES_PRIMITIVE_REGISTRY_H_
#define PRIMITIVES_PRIMITIVE_REGISTRY_H_

#include "Primitive.h"

#include <memory>
#include <string>
#include <vector>

namespace PP {
namespace primitives {

class PrimitiveRegistry {
public:
    static PrimitiveRegistry& instance();

    void registerBuiltin(std::unique_ptr<Primitive> primitive);

    /** Register all built-in probe-safe primitives. Idempotent. */
    void registerBuiltins();

    std::vector<Primitive*> list() const;
    std::vector<Primitive*> list(PrimitiveCategory category) const;
    Primitive* findByName(const std::string& name) const;

private:
    PrimitiveRegistry();
    PrimitiveRegistry(const PrimitiveRegistry&);
    PrimitiveRegistry& operator=(const PrimitiveRegistry&);

    std::vector<std::unique_ptr<Primitive> > mPrimitives;
    bool mBuiltinsRegistered;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_PRIMITIVE_REGISTRY_H_ */
