/*
 * JailbreakPlanner.h
 *
 * Device scan → jailbreak strategy selection → Gen0Options.
 * Deterministic planner for doctor/agent; LLM orchestration calls this API.
 */

#ifndef JAILBREAK_PLANNER_H_
#define JAILBREAK_PLANNER_H_

#include "DeviceState.h"
#include "Gen0Workflow.h"
#include "RamdiskTypes.h"
#include "primitives/Gen6Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PP {

class DeviceManager;

/** Snapshot from USB scan (syringe / lockdown / irecv). */
struct DeviceProfile {
    DeviceState state = DeviceState::Unknown;
    uint32_t cpid = 0;
    uint64_t ecid = 0;
    std::string udid;
    std::string productType;
    std::string iosVersion;
    std::string socName;
    bool checkm8Supported = false;
    bool usbliter8Supported = false;
    primitives::JailbreakGeneration era = primitives::JailbreakGeneration::Unknown;
};

/** Host-side readiness for a planned strategy. */
struct HostCapabilities {
    bool pluginsEnabled = false;
    bool bootModuleBuilt = false;
    std::string bootModulePath;
    bool libusbLinked = false;
};

/** Selected strategy and filled options for execution. */
struct JailbreakPlan {
    std::string strategyId;
    std::string summary;
    BootDeliveryLane bootLane = BootDeliveryLane::Auto;
    bool canProbe = true;
    bool canExecute = false;
    bool useBootromExploit = false;
    bool useBootDelivery = false;
    bool useExternalDelegate = false;
    bool useEraChain = false;
    bool useRecoveryChain = false;
    bool alreadyJailbroken = false;
    std::vector<std::string> blockers;
    std::string bootModulePath;
    /** Host IPSW used to pack ramdisk when no --ramdisk artifact is set. */
    std::string ipswPath;
    /** How ramdisk will be sourced: artifact, ipsw-pack, or unset. */
    std::string ramdiskSource;
    Gen0Options options;
};

HostCapabilities probeHostCapabilities();
bool buildDeviceProfile(DeviceManager& manager,
                        const std::string& targetUDID,
                        DeviceProfile* profile);
JailbreakPlan planJailbreak(const DeviceProfile& profile, const HostCapabilities& host);
void mergePlanIntoOptions(Gen0Options* options, const JailbreakPlan& plan);
std::string jailbreakPlanToJson(const DeviceProfile& profile, const JailbreakPlan& plan);

} /* namespace PP */

#endif /* JAILBREAK_PLANNER_H_ */
