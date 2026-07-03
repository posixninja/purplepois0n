/*
 * DoctorReporter.cpp
 */

#include "DoctorReporter.h"

#include <iostream>
#include <sstream>

namespace PP {
namespace {

std::string jsonEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (const char c : text) {
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

} /* namespace */

DoctorReporter& DoctorReporter::instance() {
    static DoctorReporter reporter;
    return reporter;
}

void DoctorReporter::setEnabled(const bool enabled) {
    mEnabled = enabled;
}

void DoctorReporter::emitLine(const std::string& json) {
    std::cout << json << std::endl;
    std::cout.flush();
}

void DoctorReporter::stepRequest(const std::string& stepId, const std::string& detail) {
    if (!mEnabled) {
        return;
    }
    std::ostringstream oss;
    oss << "{\"type\":\"step\",\"id\":\"" << jsonEscape(stepId)
        << "\",\"phase\":\"request\",\"detail\":\"" << jsonEscape(detail) << "\"}";
    emitLine(oss.str());
}

void DoctorReporter::stepResponse(const std::string& stepId,
                                  const bool success,
                                  const std::string& detail,
                                  const uint32_t status) {
    if (!mEnabled) {
        return;
    }
    std::ostringstream oss;
    oss << "{\"type\":\"step\",\"id\":\"" << jsonEscape(stepId)
        << "\",\"phase\":\"response\",\"success\":" << (success ? "true" : "false")
        << ",\"status\":" << status << ",\"detail\":\"" << jsonEscape(detail) << "\"}";
    emitLine(oss.str());
}

void DoctorReporter::syringeRequest(const std::string& transport, const std::string& command) {
    if (!mEnabled) {
        return;
    }
    std::ostringstream oss;
    oss << "{\"type\":\"syringe\",\"phase\":\"request\",\"transport\":\"" << jsonEscape(transport)
        << "\",\"command\":\"" << jsonEscape(command) << "\"}";
    emitLine(oss.str());
}

void DoctorReporter::syringeResponse(const std::string& transport,
                                     const bool success,
                                     const uint32_t status,
                                     const std::string& detail) {
    if (!mEnabled) {
        return;
    }
    std::ostringstream oss;
    oss << "{\"type\":\"syringe\",\"phase\":\"response\",\"transport\":\"" << jsonEscape(transport)
        << "\",\"success\":" << (success ? "true" : "false") << ",\"status\":" << status
        << ",\"detail\":\"" << jsonEscape(detail) << "\"}";
    emitLine(oss.str());
}

void DoctorReporter::complete(const bool success, const std::string& detail) {
    if (!mEnabled) {
        return;
    }
    std::ostringstream oss;
    oss << "{\"type\":\"complete\",\"success\":" << (success ? "true" : "false")
        << ",\"detail\":\"" << jsonEscape(detail) << "\"}";
    emitLine(oss.str());
}

} /* namespace PP */
