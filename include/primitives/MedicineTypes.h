/*
 * MedicineTypes.h
 *
 * Post-install "cures" modeled on Chronic-Dev GreenPois0n medicine
 * (afc2add, capable, sachet, loader).
 */

#ifndef PRIMITIVES_MEDICINE_TYPES_H_
#define PRIMITIVES_MEDICINE_TYPES_H_

#include <string>
#include <vector>

namespace PP {
namespace primitives {

/** Individual cure identifiers (Chronic-Dev/medicine components). */
enum class MedicineCureId {
    ActivationProbe,
    Afc2Unactivated,
    CapableStrip,
    SachetRegister,
    LoaderHint
};

struct MedicineCureStep {
    MedicineCureId id = MedicineCureId::ActivationProbe;
    std::string name;
    std::string summary;
    std::string legacyReference;
    bool selected = false;
    bool requiresSsh = false;
    /** False when iOS era / paths make automatic apply unsafe or unsupported. */
    bool autoApplicable = false;
};

struct MedicineProfile {
    std::string activationState;
    bool activated = true;
    std::string productType;
    std::string iosVersion;
    std::string jbroot;
    std::string medicineRoot;
    std::vector<MedicineCureStep> plan;
};

const char* medicineCureIdToString(MedicineCureId id);
std::vector<MedicineCureId> parseMedicineCureList(const std::string& csv);
std::vector<MedicineCureId> defaultMedicineCures();

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_MEDICINE_TYPES_H_ */
