/*
 * Gen1HistoricalModules.h
 */

#ifndef PRIMITIVES_GEN1_HISTORICAL_MODULES_H_
#define PRIMITIVES_GEN1_HISTORICAL_MODULES_H_

#include "HistoricalExploitModuleBase.h"

namespace PP {
namespace primitives {

class Evasi0nExploitModule : public HistoricalExploitModuleBase {
public:
    Evasi0nExploitModule();

protected:
    bool supportsContext(const ExecutionContext& context) const override;
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_GEN1_HISTORICAL_MODULES_H_ */
