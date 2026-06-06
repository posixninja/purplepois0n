/*
 * TssClient.h
 *
 * In-tree live TSS: libtatsu (optional) + ipsw fallback + libplist IM4M extract.
 * libtatsu is the libimobiledevice successor to idevicerestore tss.c (Joshua Hill lineage).
 */

#ifndef TSS_CLIENT_H_
#define TSS_CLIENT_H_

#include "primitives/PrimitiveTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PP {

/** Device fields for a live TSS request (Recovery / DFU irecv). */
struct TssLiveDeviceParams {
    uint64_t ecid = 0;
    uint32_t cpid = 0;
    uint32_t boardId = 0;
    std::vector<uint8_t> apNonce;
    std::string productType;
    std::string iosVersion;
};

class TssClient {
public:
    /** True when built with PURPLEPOIS0N_HAVE_LIBTATSU. */
    static bool isLibtatsuLinked();

    /** Fill live params from ExecutionContext + optional Recovery irecv nonce. */
    static TssLiveDeviceParams paramsFromContext(const primitives::ExecutionContext& context);

    /** Save SHSH blob via `ipsw download tss` (works without libtatsu). */
    static bool saveShshViaIpsw(const primitives::ExecutionContext& context,
                                const std::string& outputPath);

    /** Extract ApImg4Ticket bytes to @p im4mPath using libplist (no ipsw). */
    static bool extractIm4mFromShshPlist(const std::string& shshPath,
                                         const std::string& im4mPath);

    /**
     * Live ticket fetch: libtatsu when linked (IPSW BuildManifest + device params),
     * else ipsw download tss. Writes SHSH plist to @p shshPath.
     */
    static bool fetchLiveShsh(const primitives::ExecutionContext& context,
                            const std::string& shshPath);

    /** fetchLiveShsh then extract IM4M to @p im4mPath. */
    static bool fetchLiveIm4m(const primitives::ExecutionContext& context,
                              const std::string& shshPath,
                              const std::string& im4mPath);

    /** Log futurerestore SEP/baseband ticket strategy (probe). */
    static void logFuturerestoreSepBbPlan(const primitives::ExecutionContext& context);
};

} /* namespace PP */

#endif /* TSS_CLIENT_H_ */
