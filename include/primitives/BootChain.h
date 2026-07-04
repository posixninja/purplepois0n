/*
 * BootChain.h
 *
 * Lane-dispatching boot delivery chain. Ramdisk artifacts are independent of payload/transport.
 */

#ifndef PRIMITIVES_BOOT_CHAIN_H_
#define PRIMITIVES_BOOT_CHAIN_H_

#include "primitives/PrimitiveTypes.h"

namespace PP {
namespace primitives {

class ChainRunner;

/** Probe/execute boot delivery for the resolved lane (recovery, usb-loader, live-agent, …). */
bool runBootDeliveryChain(ExecutionContext& context, bool executeMode, ChainRunner& runner);

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_BOOT_CHAIN_H_ */
