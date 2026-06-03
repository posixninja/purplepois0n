/*
 * AnalysisPayload.h — JSON-ish payload helpers for opaque analysis objects.
 */

#ifndef ANALYSIS_PAYLOAD_H_
#define ANALYSIS_PAYLOAD_H_

#include <string>

namespace PP {

/** True if @p text looks like JSON object/array from an external tool. */
bool looksLikeJson(const std::string& text);

/** Escape a string for JSON double-quoted value. */
std::string jsonEscape(const std::string& value);

/** Write @p json to path (creates/truncates). Returns false on I/O error. */
bool writePayloadFile(const std::string& path, const std::string& json);

} /* namespace PP */

#endif /* ANALYSIS_PAYLOAD_H_ */
