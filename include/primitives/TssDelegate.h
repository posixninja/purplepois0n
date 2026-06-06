/*
 * TssDelegate.h
 *
 * TSS signing probes and external tool orchestration (idevicerestore, futurerestore, ipsw).
 */

#ifndef PRIMITIVES_TSS_DELEGATE_H_
#define PRIMITIVES_TSS_DELEGATE_H_

#include "TssTypes.h"
#include "PrimitiveTypes.h"

#include <string>
#include <vector>

namespace PP {
namespace primitives {

class TssDelegate {
public:
    static std::string findIdevicerestore();
    static std::string findFuturerestore();
    static std::string findIpsw();

    static bool isIdevicerestoreConfigured();
    static bool isFuturerestoreConfigured();
    static bool isIpswConfigured();

    /** Resolved signing mode from context + env. */
    static TssSigningMode resolveSigningMode(const ExecutionContext& context);

    /**
     * Log stock idevicerestore vs futurerestore process differences and tool availability.
     */
    static void logProcessOverview(const ExecutionContext& context);

    /**
     * Build futurerestore argv for @p ipswPath (does not execute unless @p execute is true).
     * Requires apticket for futurerestore path.
     */
    static std::vector<std::string> buildFuturerestoreArgv(const ExecutionContext& context,
                                                           const std::string& ipswPath);

    /**
     * Check whether Apple still signs a build (host-only via ipsw download tss --signed).
     */
    static PrimitiveResult checkStillSigned(const ExecutionContext& context);

    /** Probe: overview + optional signing check when device metadata present. */
    static PrimitiveResult probe(const ExecutionContext& context);

    /**
     * Spawn futurerestore restore (mutation only). Full unsigned restore — use with care.
     */
    static PrimitiveResult runFuturerestoreRestore(const ExecutionContext& context,
                                                   bool allowMutation);

    /** Extract IM4M manifest from APTicket via `ipsw img4 im4m extract`. */
    static PrimitiveResult extractIm4mFromApticket(const std::string& apticketPath,
                                                   const std::string& outputPath);

    /**
     * Personalize unsigned component with IM4M via `ipsw img4 person`.
     * @p componentFourcc e.g. "iBSS", "iBEC", "krnl" (optional).
     */
    static PrimitiveResult personalizeComponent(const std::string& unsignedComponentPath,
                                                const std::string& manifestPath,
                                                const std::string& outputPath,
                                                const std::string& componentFourcc = "");

    /** Live SHSH fetch (libtatsu when linked, else ipsw download tss). */
    static PrimitiveResult fetchLiveShsh(const ExecutionContext& context,
                                         const std::string& outputPath);

    /** Live SHSH + IM4M extract for Recovery personalize path. */
    static PrimitiveResult fetchLiveIm4m(const ExecutionContext& context,
                                         const std::string& shshPath,
                                         const std::string& im4mPath);

    static bool isLibtatsuLinked();
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_TSS_DELEGATE_H_ */
