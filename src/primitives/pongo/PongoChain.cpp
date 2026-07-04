/*
 * PongoChain.cpp
 *
 * Legacy entry point — delegates to lane-dispatching BootChain (usb-loader lane).
 */

#include "primitives/pongo/PongoChain.h"
#include "primitives/BootChain.h"
#include "primitives/ChainRunner.h"
#include "RamdiskTypes.h"

namespace PP {
namespace primitives {

bool runPongoChain(ExecutionContext& context, bool executeMode, ChainRunner& runner) {
    if (context.bootDeliveryLane == BootDeliveryLane::Auto) {
        context.bootDeliveryLane = BootDeliveryLane::UsbLoader;
    }
    return runBootDeliveryChain(context, executeMode, runner);
}

} /* namespace primitives */
} /* namespace PP */
