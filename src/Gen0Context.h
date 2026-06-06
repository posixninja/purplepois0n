/*
 * Gen0Context.h
 */

#ifndef GEN0_CONTEXT_H_
#define GEN0_CONTEXT_H_

#include "Gen0Workflow.h"
#include "primitives/PrimitiveTypes.h"

#include <cstdint>
#include <string>

namespace PP {

primitives::ExecutionContext buildExecutionContext(DeviceState state,
                                                   const Gen0Options& options,
                                                   const std::string& udid,
                                                   uint64_t ecid,
                                                   bool allowMutation);

} /* namespace PP */

#endif /* GEN0_CONTEXT_H_ */
