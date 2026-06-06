/*
 * EnvUtil.h
 *
 * Shared PURPLEPOIS0N_* environment helpers.
 */

#ifndef ENV_UTIL_H_
#define ENV_UTIL_H_

#include <cstdint>
#include <string>

namespace PP {

/** Non-empty env value, or empty string. */
std::string envOrEmpty(const char* name);

/** True when @p name is set and not "0". */
bool envFlagEnabled(const char* name);

/** True when unset/empty → @p defaultValue; otherwise value[0] != '0'. */
bool truthyEnv(const char* name, bool defaultValue);

/** Parse port from env; returns @p fallback on missing/invalid. */
uint16_t parsePortEnv(const char* name, uint16_t fallback);

} /* namespace PP */

#endif /* ENV_UTIL_H_ */
