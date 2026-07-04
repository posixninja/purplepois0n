/*
 * DoctorWorkflow.cpp
 */

#include "DoctorWorkflow.h"

#include "Checkm8.h"
#include "DeviceManager.h"
#include "DoctorReporter.h"
#include "JailbreakPlanner.h"
#include "Logger.h"
#include "Syringe.h"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace PP {
namespace {

const char* deviceStateLabel(const DeviceState state) {
    switch (state) {
        case DeviceState::DFU:
            return "DFU";
        case DeviceState::Recovery:
            return "Recovery";
        case DeviceState::Normal:
            return "Normal";
        default:
            return "Unknown";
    }
}

const char* syringeTransportLabel(const SyringeTransport transport) {
    switch (transport) {
        case SyringeTransport::DFU:
            return "dfu";
        case SyringeTransport::Recovery:
            return "recovery";
        case SyringeTransport::Normal:
            return "normal";
    }
    return "unknown";
}

bool syringeIdentify(Syringe& syringe, uint32_t& cpid, uint64_t& ecid) {
    auto& reporter = DoctorReporter::instance();
    const std::string transport = syringeTransportLabel(syringe.currentTransport());

    reporter.syringeRequest(transport, "getenv cpid");
    if (!syringe.queryCpid(cpid)) {
        reporter.syringeResponse(transport, false, 1, "cpid query failed");
        return false;
    }
    reporter.syringeResponse(transport, true, 0,
                             "cpid=0x" + ([&]() {
                                 std::ostringstream oss;
                                 oss << std::hex << cpid;
                                 return oss.str();
                             })());

    reporter.syringeRequest(transport, "getenv ecid");
    if (!syringe.queryEcid(ecid)) {
        reporter.syringeResponse(transport, false, 1, "ecid query failed");
        return false;
    }
    reporter.syringeResponse(transport, true, 0, "ecid=" + std::to_string(ecid));
    return true;
}

} /* namespace */

bool runDevicePlanScan(DeviceManager& manager,
                       const std::string& targetUDID,
                       std::string* jsonOut) {
    DeviceProfile profile;
    if (!buildDeviceProfile(manager, targetUDID, &profile)) {
        return false;
    }
    const HostCapabilities host = probeHostCapabilities();
    const JailbreakPlan plan = planJailbreak(profile, host);
    if (jsonOut != nullptr) {
        *jsonOut = jailbreakPlanToJson(profile, plan);
    }
    return true;
}

bool runDoctorFlow(DeviceManager& manager,
                   const std::string& targetUDID,
                   const Gen0Options& baseOptions) {
    Gen0Options options = baseOptions;
    auto& reporter = DoctorReporter::instance();
    reporter.setEnabled(true);

    reporter.stepRequest("detect", "Waiting for USB device");
    const DeviceState state = manager.detectDeviceState(targetUDID);
    if (state == DeviceState::Unknown) {
        reporter.stepResponse("detect", false, "No device detected", 1);
        reporter.complete(false, "Connect an iOS device and enter DFU or Recovery");
        return false;
    }
    reporter.stepResponse("detect", true, deviceStateLabel(state), 0);

    reporter.stepRequest("syringe-connect", "Opening syringe transport");
    Syringe syringe;
    if (!syringe.detectDevice(manager)) {
        reporter.stepResponse("syringe-connect", false, "Syringe connect failed", 1);
        reporter.complete(false, "Could not open device transport");
        return false;
    }
    reporter.stepResponse("syringe-connect", true,
                          syringeTransportLabel(syringe.currentTransport()), 0);

    reporter.stepRequest("identify", "Query CPID and ECID");
    uint32_t cpid = 0;
    uint64_t ecid = 0;
    if (!syringeIdentify(syringe, cpid, ecid)) {
        reporter.stepResponse("identify", false, "Device identify failed", 1);
        reporter.complete(false, "Could not read device identifiers");
        return false;
    }
    reporter.stepResponse("identify", true,
                          std::string(Checkm8::cpidToSocName(cpid)) + " ecid=" +
                              std::to_string(ecid),
                          0);

    reporter.stepRequest("plan", "Selecting jailbreak strategy for this device");
    DeviceProfile profile;
    if (!buildDeviceProfile(manager, targetUDID, &profile)) {
        reporter.stepResponse("plan", false, "Could not build device profile", 1);
        reporter.complete(false, "Device profile failed");
        return false;
    }
    const HostCapabilities host = probeHostCapabilities();
    const JailbreakPlan plan = planJailbreak(profile, host);
    reporter.stepResponse("plan", plan.canProbe || plan.canExecute, plan.summary, 0);
    std::cout << jailbreakPlanToJson(profile, plan) << std::endl;

    const bool userWantsExecute = options.jailbreakExecute;
    mergePlanIntoOptions(&options, plan);
    if (!targetUDID.empty() && options.ramdisk.connect.udid.empty()) {
        options.ramdisk.connect.udid = targetUDID;
    }
    if (userWantsExecute) {
        options.jailbreakExecute = true;
        if (plan.useRecoveryChain) {
            options.recovery.execute = true;
        }
    }

    if (options.jailbreakExecute && !plan.canExecute) {
        std::string blockers = plan.blockers.empty() ? "plan not executable" : plan.blockers[0];
        reporter.stepResponse("jailbreak", false, blockers, 1);
        reporter.complete(false, blockers);
        return false;
    }

    if (!options.jailbreakExecute) {
        reporter.complete(true, "Plan ready");
        return true;
    }

    reporter.stepRequest("jailbreak", "Running " + plan.strategyId);
    bool ok = false;
    if (profile.state == DeviceState::DFU) {
        ok = runDfuJailbreak(manager, options, options.jailbreakExecute);
    } else if (plan.useExternalDelegate) {
        ok = runExternalJailbreak(manager, targetUDID, options);
    } else {
        ok = runGen0Jailbreak(manager, targetUDID, options);
    }
    reporter.stepResponse("jailbreak", ok,
                          ok ? "Chain finished" : "Chain failed — see log",
                          ok ? 0u : 1u);

    reporter.complete(ok, ok ? "Doctor run complete" : "Doctor run failed");
    return ok;
}

} /* namespace PP */
