/*
 * PrimitiveRegistry.cpp
 */

#include "primitives/PrimitiveRegistry.h"
#include "primitives/Checkm8BootromPrimitive.h"
#include "primitives/OfflinePatchPrimitive.h"
#include "primitives/AfcInjectionPrimitive.h"
#include "primitives/NormalModeProbePrimitive.h"

namespace PP {
namespace primitives {

PrimitiveRegistry::PrimitiveRegistry() : mBuiltinsRegistered(false) {}

PrimitiveRegistry& PrimitiveRegistry::instance() {
    static PrimitiveRegistry registry;
    return registry;
}

void PrimitiveRegistry::registerBuiltin(std::unique_ptr<Primitive> primitive) {
    if (primitive.get() == nullptr) {
        return;
    }
    mPrimitives.push_back(std::move(primitive));
}

void PrimitiveRegistry::registerBuiltins() {
    if (mBuiltinsRegistered) {
        return;
    }
    registerBuiltin(std::unique_ptr<Primitive>(new Checkm8BootromPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new OfflinePatchPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new AfcInjectionPrimitive()));
    registerBuiltin(std::unique_ptr<Primitive>(new NormalModeProbePrimitive()));
    mBuiltinsRegistered = true;
}

std::vector<Primitive*> PrimitiveRegistry::list() const {
    std::vector<Primitive*> out;
    for (size_t i = 0; i < mPrimitives.size(); ++i) {
        out.push_back(mPrimitives[i].get());
    }
    return out;
}

std::vector<Primitive*> PrimitiveRegistry::list(PrimitiveCategory category) const {
    std::vector<Primitive*> out;
    for (size_t i = 0; i < mPrimitives.size(); ++i) {
        if (mPrimitives[i]->category() == category) {
            out.push_back(mPrimitives[i].get());
        }
    }
    return out;
}

Primitive* PrimitiveRegistry::findByName(const std::string& name) const {
    for (size_t i = 0; i < mPrimitives.size(); ++i) {
        if (name == mPrimitives[i]->name()) {
            return mPrimitives[i].get();
        }
    }
    return nullptr;
}

} /* namespace primitives */
} /* namespace PP */
