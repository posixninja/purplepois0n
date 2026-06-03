/*
 * Checkm8BootromPrimitive.cpp
 */

#include "primitives/Checkm8BootromPrimitive.h"
#include "primitives/DfuTransport.h"
#include "Logger.h"

#include <sstream>

namespace PP {
namespace primitives {

const char* Checkm8BootromPrimitive::name() const {
    return "checkm8-bootrom";
}

std::vector<PrimitiveOperation> Checkm8BootromPrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe, PrimitiveOperation::Execute};
}

std::vector<DeviceState> Checkm8BootromPrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::DFU};
}

bool Checkm8BootromPrimitive::canRun(const ExecutionContext& context) const {
    if (context.deviceState != DeviceState::DFU) {
        return false;
    }
    return context.transport != nullptr;
}

PrimitiveResult Checkm8BootromPrimitive::execute(ExecutionContext& context) {
    DfuTransport* dfuTransport = dynamic_cast<DfuTransport*>(context.transport);
    if (dfuTransport == nullptr) {
        return PrimitiveResult::TransportError;
    }

    if (!context.allowMutation) {
        mLastProbe = Checkm8::probe(dfuTransport->device());
        mHasProbe = true;
        context.cpid = mLastProbe.cpid;

        std::ostringstream oss;
        oss << "CPID 0x" << std::hex << mLastProbe.cpid << std::dec
            << " (" << mLastProbe.socName << ")"
            << " supported=" << (mLastProbe.supported ? "yes" : "no")
            << " pwned=" << (mLastProbe.pwned ? "yes" : "no");
        Logger::info(std::string("  [Bootrom] ") + oss.str());

        if (!mLastProbe.supported) {
            return PrimitiveResult::Unsupported;
        }
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [Bootrom] Execute blocked — rebuild with PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS");
        return PrimitiveResult::PluginDisabled;
    }

    if (!mHasProbe) {
        mLastProbe = Checkm8::probe(dfuTransport->device());
        mHasProbe = true;
        context.cpid = mLastProbe.cpid;
    }

    if (!mLastProbe.supported) {
        return PrimitiveResult::Unsupported;
    }

    const Checkm8Result result = Checkm8::runExploit(mLastProbe, true);
    Logger::info(std::string("  [Bootrom] checkm8: ") + Checkm8::resultToString(result));
    return (result == Checkm8Result::Success || result == Checkm8Result::AlreadyPwned)
               ? PrimitiveResult::Success
               : PrimitiveResult::Failed;
}

} /* namespace primitives */
} /* namespace PP */
