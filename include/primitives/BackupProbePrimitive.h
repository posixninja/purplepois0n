/*
 * BackupProbePrimitive.h
 *
 * Offline backup manifest summary inside ChainRunner Probe stage.
 */

#ifndef PRIMITIVES_BACKUP_PROBE_PRIMITIVE_H_
#define PRIMITIVES_BACKUP_PROBE_PRIMITIVE_H_

#include "Primitive.h"

namespace PP {
namespace primitives {

class BackupProbePrimitive : public Primitive {
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

#endif /* PRIMITIVES_BACKUP_PROBE_PRIMITIVE_H_ */
