/*
 * IpswdHostProbePrimitive.cpp
 */

#include "primitives/IpswdHostProbePrimitive.h"
#include "IpswdClient.h"
#include "Logger.h"

namespace PP {
namespace primitives {

const char* IpswdHostProbePrimitive::name() const {
    return "ipswd-host-probe";
}

std::vector<PrimitiveOperation> IpswdHostProbePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe};
}

std::vector<DeviceState> IpswdHostProbePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Unknown,
                                    DeviceState::Normal,
                                    DeviceState::Recovery,
                                    DeviceState::DFU};
}

bool IpswdHostProbePrimitive::canRun(const ExecutionContext& /*context*/) const {
    return true;
}

std::string IpswdHostProbePrimitive::targetKind() const {
    return "firmware-analysis";
}

PrimitiveResult IpswdHostProbePrimitive::execute(ExecutionContext& /*context*/) {
    const std::string baseUrl = IpswdClient::defaultBaseUrl();
    if (IpswdClient::ping(baseUrl)) {
        Logger::info("  [Firmware] ipswd reachable at " + baseUrl);
    } else {
        Logger::info("  [Firmware] ipswd not running — MachOBinary/DyldSharedCache fall back to ipsw CLI");
    }
    Logger::info("  [Firmware] offline: --analyze-binary, --analyze-dyldcache, --analyze-json");
    Logger::info("  [Firmware] see docs/BOOGERAIDS.md and book/deep/binary-parsers.md");
    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
