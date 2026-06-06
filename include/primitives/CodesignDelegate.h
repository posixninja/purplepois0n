/*
 * CodesignDelegate.h
 *
 * Host-side Mach-O / .app codesign via ipsw macho sign and optional ldid.
 */

#ifndef PRIMITIVES_CODESIGN_DELEGATE_H_
#define PRIMITIVES_CODESIGN_DELEGATE_H_

#include "CodesignTypes.h"
#include "PrimitiveTypes.h"

#include <string>
#include <vector>

namespace PP {
namespace primitives {

class CodesignDelegate {
public:
    static std::string findIpsw();
    static std::string findLdid();
    static std::string findCodesign();

    static bool isIpswConfigured();
    static bool isLdidConfigured();

    static CodesignOptions mergeOptions(const CodesignOptions& cli, const CodesignOptions& env);

    static void logToolOverview();

    static std::vector<std::string> buildIpswSignArgv(const CodesignOptions& opts,
                                                      const std::string& inputPath);

    /** Sign with ldid -S$ent $binary when configured. */
    static PrimitiveResult signWithLdid(const CodesignOptions& opts, const std::string& binaryPath);

    /** ipsw macho sign (Mach-O file or .app bundle directory). */
    static PrimitiveResult signWithIpsw(const CodesignOptions& opts, const std::string& inputPath);

    /** ldid (optional) then ipsw; probe logs plan when @p allowMutation is false. */
    static PrimitiveResult signPath(const CodesignOptions& opts,
                                    const std::string& inputPath,
                                    bool allowMutation);

    static PrimitiveResult probe(const ExecutionContext& context);
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_CODESIGN_DELEGATE_H_ */
