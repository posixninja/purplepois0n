/*
 * IpaSignHelper.h
 *
 * Unpack IPA, sign Payload .app bundle via CodesignDelegate, repack.
 */

#ifndef IPA_SIGN_HELPER_H_
#define IPA_SIGN_HELPER_H_

#include <string>

namespace PP {
namespace primitives {
struct CodesignOptions;
}

/** @return signed IPA path written, or empty on failure. */
std::string signIpa(const primitives::CodesignOptions& opts,
                    const std::string& ipaPath,
                    const std::string& outputIpaPath,
                    bool allowMutation);

} /* namespace PP */

#endif /* IPA_SIGN_HELPER_H_ */
