/*
 * Primitive.h
 *
 * Abstract base for all exploit/research primitives.
 */

#ifndef PRIMITIVES_PRIMITIVE_H_
#define PRIMITIVES_PRIMITIVE_H_

#include "PrimitiveTypes.h"

#include <string>
#include <vector>

namespace PP {
namespace primitives {

/**
 * @class Primitive
 * @brief Base class for domain-specific primitives (bootrom, kernel, etc.).
 */
class Primitive {
public:
    virtual ~Primitive() {}

    virtual const char* name() const = 0;
    virtual PrimitiveCategory category() const = 0;
    virtual std::vector<PrimitiveOperation> supportedOperations() const = 0;

    virtual bool supports(PrimitiveOperation op) const;
    virtual std::vector<DeviceState> requiredDeviceStates() const = 0;
    virtual bool canRun(const ExecutionContext& context) const = 0;

    /** Gen6 ordered chain stage; ChainStage::Probe if not part of Dopamine-shaped chain. */
    virtual ChainStage gen6Stage() const { return ChainStage::Probe; }

    virtual PrimitiveResult execute(ExecutionContext& context) = 0;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_PRIMITIVE_H_ */
