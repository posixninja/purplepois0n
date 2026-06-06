/*
 * Gen0HistoricalModules.h
 */

#ifndef PRIMITIVES_GEN0_HISTORICAL_MODULES_H_
#define PRIMITIVES_GEN0_HISTORICAL_MODULES_H_

#include "HistoricalExploitModuleBase.h"

namespace PP {
namespace primitives {

class Limera1nExploitModule : public HistoricalExploitModuleBase {
public:
    Limera1nExploitModule();

    std::vector<DeviceState> requiredDeviceStates() const override;
    bool canRun(const ExecutionContext& context) const override;

protected:
    bool supportsContext(const ExecutionContext& context) const override;
};

class Kpwn24kExploitModule : public HistoricalExploitModuleBase {
public:
    Kpwn24kExploitModule();

    std::vector<DeviceState> requiredDeviceStates() const override;
    bool canRun(const ExecutionContext& context) const override;

protected:
    bool supportsContext(const ExecutionContext& context) const override;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_GEN0_HISTORICAL_MODULES_H_ */
