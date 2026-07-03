/*
 * MedicineTypes.cpp
 */

#include "primitives/MedicineTypes.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace PP {
namespace primitives {

namespace {

std::string trimCopy(std::string text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.erase(text.begin());
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.pop_back();
    }
    return text;
}

MedicineCureId parseOneCure(const std::string& token) {
    const std::string lower = trimCopy(token);
    if (lower == "activation" || lower == "probe") {
        return MedicineCureId::ActivationProbe;
    }
    if (lower == "afc2" || lower == "afc2add") {
        return MedicineCureId::Afc2Unactivated;
    }
    if (lower == "capable" || lower == "capability") {
        return MedicineCureId::CapableStrip;
    }
    if (lower == "sachet" || lower == "register-app") {
        return MedicineCureId::SachetRegister;
    }
    if (lower == "loader") {
        return MedicineCureId::LoaderHint;
    }
    if (lower == "all" || lower == "default") {
        return MedicineCureId::Afc2Unactivated;
    }
    return MedicineCureId::ActivationProbe;
}

} /* anonymous */

const char* medicineCureIdToString(MedicineCureId id) {
    switch (id) {
        case MedicineCureId::ActivationProbe:
            return "activation-probe";
        case MedicineCureId::Afc2Unactivated:
            return "afc2-unactivated";
        case MedicineCureId::CapableStrip:
            return "capable-strip";
        case MedicineCureId::SachetRegister:
            return "sachet-register";
        case MedicineCureId::LoaderHint:
            return "loader-hint";
    }
    return "unknown";
}

std::vector<MedicineCureId> parseMedicineCureList(const std::string& csv) {
    std::vector<MedicineCureId> out;
    if (csv.empty()) {
        return defaultMedicineCures();
    }
    std::istringstream stream(csv);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trimCopy(token);
        if (token.empty()) {
            continue;
        }
        if (token == "all" || token == "default") {
            const std::vector<MedicineCureId> defaults = defaultMedicineCures();
            out.insert(out.end(), defaults.begin(), defaults.end());
            continue;
        }
        out.push_back(parseOneCure(token));
    }
    if (out.empty()) {
        return defaultMedicineCures();
    }
    return out;
}

std::vector<MedicineCureId> defaultMedicineCures() {
    return std::vector<MedicineCureId>{MedicineCureId::Afc2Unactivated, MedicineCureId::CapableStrip,
                                       MedicineCureId::LoaderHint};
}

} /* namespace primitives */
} /* namespace PP */
