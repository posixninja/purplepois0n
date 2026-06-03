/*
 * AnalysisPayload.cpp
 */

#include "AnalysisPayload.h"

#include <fstream>

namespace PP {

bool looksLikeJson(const std::string& text) {
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            continue;
        }
        return c == '{' || c == '[';
    }
    return false;
}

std::string jsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

bool writePayloadFile(const std::string& path, const std::string& json) {
    std::ofstream out(path.c_str(), std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << json;
    return out.good();
}

} /* namespace PP */
