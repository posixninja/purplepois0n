/*
 * TrustCacheDelegate.h
 *
 * External jbctl / trustcache tool for post-jailbreak pseudo-signing.
 */

#ifndef PRIMITIVES_TRUSTCACHE_DELEGATE_H_
#define PRIMITIVES_TRUSTCACHE_DELEGATE_H_

#include "PrimitiveTypes.h"

#include <string>

namespace PP {
namespace primitives {

class TrustCacheDelegate {
public:
    static std::string findJbctl();
    static std::string findTrustCacheTool();
    static std::string resolveBinaryPath(const ExecutionContext& context);

    static void logToolOverview();

    static PrimitiveResult addToTrustCache(const ExecutionContext& context, bool allowMutation);
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_TRUSTCACHE_DELEGATE_H_ */
