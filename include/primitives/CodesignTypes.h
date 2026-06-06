/*
 * CodesignTypes.h
 *
 * Host-side Mach-O / IPA codesign options (sideload research).
 */

#ifndef PRIMITIVES_CODESIGN_TYPES_H_
#define PRIMITIVES_CODESIGN_TYPES_H_

#include <string>

namespace PP {
namespace primitives {

struct CodesignOptions {
    std::string bundleId;
    std::string entitlementsPath;
    std::string teamId;
    std::string p12Path;
    std::string p12Password;
    std::string outputPath;
    bool adHoc = true;
    bool overwrite = false;
};

/** Load CodesignOptions from PURPLEPOIS0N_CODESIGN_* env. */
CodesignOptions codesignOptionsFromEnv();

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_CODESIGN_TYPES_H_ */
