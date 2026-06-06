/*
 * CodesignTypes.cpp
 */

#include "primitives/CodesignTypes.h"

#include <cstdlib>
#include <cstring>

namespace PP {
namespace primitives {

namespace {

std::string envString(const char* name) {
    const char* v = std::getenv(name);
    return (v != nullptr) ? std::string(v) : std::string();
}

bool envTruthy(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0' && strcmp(v, "0") != 0;
}

} /* anonymous */

CodesignOptions codesignOptionsFromEnv() {
    CodesignOptions opts;
    opts.bundleId = envString("PURPLEPOIS0N_CODESIGN_ID");
    opts.entitlementsPath = envString("PURPLEPOIS0N_CODESIGN_ENT");
    opts.teamId = envString("PURPLEPOIS0N_CODESIGN_TEAM");
    opts.p12Path = envString("PURPLEPOIS0N_CODESIGN_P12");
    opts.p12Password = envString("PURPLEPOIS0N_CODESIGN_P12_PASSWORD");
    opts.outputPath = envString("PURPLEPOIS0N_CODESIGN_OUTPUT");
    if (envTruthy("PURPLEPOIS0N_CODESIGN_ADHOC")) {
        opts.adHoc = true;
    }
    if (!opts.p12Path.empty()) {
        opts.adHoc = false;
    }
    return opts;
}

} /* namespace primitives */
} /* namespace PP */
