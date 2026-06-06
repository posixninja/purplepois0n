/*
 * Gen5HistoricalModules.h
 */

#ifndef PRIMITIVES_GEN5_HISTORICAL_MODULES_H_
#define PRIMITIVES_GEN5_HISTORICAL_MODULES_H_

#include "HistoricalExploitModuleBase.h"

namespace PP {
namespace primitives {

class Checkra1nExploitModule : public HistoricalExploitModuleBase {
public:
    Checkra1nExploitModule();

    std::vector<DeviceState> requiredDeviceStates() const override;
    bool canRun(const ExecutionContext& context) const override;

protected:
    bool supportsContext(const ExecutionContext& context) const override;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_GEN5_HISTORICAL_MODULES_H_ */
