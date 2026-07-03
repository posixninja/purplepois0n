/*
 * MedicineDelegate.h
 *
 * Post-jailbreak fixes (Chronic-Dev medicine lineage): hacktivation helpers,
 * AFC2 on unactivated devices, SpringBoard capability stripping, app registration.
 */

#ifndef PRIMITIVES_MEDICINE_DELEGATE_H_
#define PRIMITIVES_MEDICINE_DELEGATE_H_

#include "MedicineTypes.h"
#include "PrimitiveTypes.h"

namespace PP {
namespace primitives {

class MedicineDelegate {
public:
    /** Resolve mirror path: PURPLEPOIS0N_MEDICINE_ROOT or legacy/Chronic-Dev/medicine. */
    static std::string resolveMedicineRoot();

    /** Lockdown ActivationState + product metadata (Normal mode USB). */
    static bool queryActivation(const std::string& udid, MedicineProfile* profile);

    /** Build cure plan from context (selected cures, paths, era hints). */
    static MedicineProfile buildPlan(const ExecutionContext& context);

    static void logMedicinePlan(const MedicineProfile& profile);

    /** Probe-only or SSH apply for selected cures. */
    static PrimitiveResult runMedicine(const ExecutionContext& context, bool allowMutation);
};

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_MEDICINE_DELEGATE_H_ */
