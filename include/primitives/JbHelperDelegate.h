/*
 * JbHelperDelegate.h
 *
 * Spawn external jailbreak installer/bootstrap helper (PURPLEPOIS0N_JB_HELPER).
 */

#ifndef PRIMITIVES_JB_HELPER_DELEGATE_H_
#define PRIMITIVES_JB_HELPER_DELEGATE_H_

#include "PrimitiveTypes.h"

namespace PP {
namespace primitives {

class JbHelperDelegate {
public:
    /** True when PURPLEPOIS0N_JB_HELPER is set and executable. */
    static bool isConfigured();

    /** Log helper path on probe; spawn when @p allowMutation. */
    static PrimitiveResult run(const ExecutionContext& context, bool allowMutation);
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_JB_HELPER_DELEGATE_H_ */
