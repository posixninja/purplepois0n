/*
 * EnvUtil.cpp
 */

#include "EnvUtil.h"

#include <cstdlib>
#include <cstring>

namespace PP {

std::string envOrEmpty(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return std::string();
    }
    return std::string(value);
}

bool envFlagEnabled(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0' && std::strcmp(value, "0") != 0;
}

bool truthyEnv(const char* name, bool defaultValue) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return defaultValue;
    }
    return value[0] != '0';
}

uint16_t parsePortEnv(const char* name, uint16_t fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    const long parsed = std::strtol(value, nullptr, 10);
    if (parsed <= 0 || parsed > 65535) {
        return fallback;
    }
    return static_cast<uint16_t>(parsed);
}

} /* namespace PP */
