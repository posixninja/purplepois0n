/*
 * PongoChain.h
 *
 * PongoOS probe/boot mini-chain (USB 05ac:4141).
 */

#ifndef PRIMITIVES_PONGO_CHAIN_H_
#define PRIMITIVES_PONGO_CHAIN_H_

#include "primitives/PrimitiveTypes.h"

namespace PP {
namespace primitives {

class ChainRunner;

bool runPongoChain(ExecutionContext& context, bool executeMode, ChainRunner& runner);

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_PONGO_CHAIN_H_ */
