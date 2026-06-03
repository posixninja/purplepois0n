/*
 * ChainRunner.cpp
 */

#include "primitives/ChainRunner.h"
#include "primitives/PrimitiveRegistry.h"
#include "primitives/ITransport.h"
#include "Logger.h"

#include <fstream>
#include <sstream>

namespace PP {
namespace primitives {

namespace {

constexpr const char* kSupportDoc = "docs/SUPPORT.md";

const char* deviceStateLabel(DeviceState state) {
    switch (state) {
        case DeviceState::Normal:
            return "Normal";
        case DeviceState::Recovery:
            return "Recovery";
        case DeviceState::DFU:
            return "DFU";
        default:
            return "Unknown";
    }
}

bool stateAllowed(const std::vector<DeviceState>& required, DeviceState state) {
    for (size_t i = 0; i < required.size(); ++i) {
        if (required[i] == state || required[i] == DeviceState::Unknown) {
            return true;
        }
    }
    return false;
}

} /* anonymous namespace */

ChainRunner::ChainRunner() {}

void ChainRunner::logStage(ChainStage stage, const std::string& message) {
    Logger::info(std::string("[") + stageToString(stage) + "] " + message);
}

void ChainRunner::recordReport(ChainStage stage, PrimitiveResult result, const std::string& message) {
    ChainReport report;
    report.stage = stage;
    report.result = result;
    report.message = message;
    mReports.push_back(report);
}

bool ChainRunner::runStage(ChainStage stage, ExecutionContext& context, bool executeMode) {
    PrimitiveRegistry& registry = PrimitiveRegistry::instance();
    registry.registerBuiltins();

    const std::vector<Primitive*> primitives = registry.list();
    bool anyRan = false;

    for (size_t i = 0; i < primitives.size(); ++i) {
        Primitive* primitive = primitives[i];
        if (!stateAllowed(primitive->requiredDeviceStates(), context.deviceState)) {
            continue;
        }

        if (stage == ChainStage::Probe && !primitive->supports(PrimitiveOperation::Probe)) {
            continue;
        }
        if (stage == ChainStage::Execute && !primitive->supports(PrimitiveOperation::Execute)) {
            continue;
        }

        if (!primitive->canRun(context)) {
            continue;
        }

        const bool savedMutation = context.allowMutation;
        context.allowMutation = executeMode;

        std::ostringstream prefix;
        prefix << primitive->name() << " (" << categoryToString(primitive->category()) << "): ";
        logStage(stage, prefix.str() + "running");

        const PrimitiveResult result = primitive->execute(context);
        context.allowMutation = savedMutation;

        recordReport(stage, result, prefix.str() + resultToString(result));
        anyRan = true;

        if (result == PrimitiveResult::Failed || result == PrimitiveResult::TransportError) {
            Logger::warn(prefix.str() + resultToString(result));
        }
    }

    return anyRan;
}

bool ChainRunner::runProbeChain(ExecutionContext& context) {
    mReports.clear();

    logStage(ChainStage::Detect, std::string("device mode: ") + deviceStateLabel(context.deviceState));
    if (context.cpid != 0) {
        std::ostringstream oss;
        oss << "CPID 0x" << std::hex << context.cpid << std::dec;
        logStage(ChainStage::Detect, oss.str());
    }
    recordReport(ChainStage::Detect, PrimitiveResult::Success, "mode detected");

    if (context.transport != nullptr) {
        logStage(ChainStage::Connect,
                 std::string("transport: ") + context.transport->transportName());
        recordReport(ChainStage::Connect, PrimitiveResult::Success, "transport attached");
    } else {
        logStage(ChainStage::Connect, "no live transport (offline probes only)");
        recordReport(ChainStage::Connect, PrimitiveResult::NotApplicable, "no transport");
    }

    context.allowMutation = false;
    runStage(ChainStage::Probe, context, false);

    logStage(ChainStage::Report, std::string("see ") + kSupportDoc + " for capability gaps");
    recordReport(ChainStage::Report, PrimitiveResult::Success, "probe chain complete");

    return true;
}

bool ChainRunner::runExecuteChain(ExecutionContext& context) {
    if (!context.allowMutation) {
        logStage(ChainStage::Execute, "mutation not allowed — use -m/--checkm8");
        recordReport(ChainStage::Execute, PrimitiveResult::ProbeOnly, "mutation disabled");
        return false;
    }

    if (!exploitPluginsEnabled()) {
        logStage(ChainStage::Execute,
                 "PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS not set at compile time");
        recordReport(ChainStage::Execute, PrimitiveResult::PluginDisabled, "plugins disabled");
        return false;
    }

    logStage(ChainStage::Execute, "running mutating primitives");
    const bool ran = runStage(ChainStage::Execute, context, true);
    recordReport(ChainStage::Execute,
                 ran ? PrimitiveResult::Success : PrimitiveResult::NotApplicable,
                 "execute chain finished");
    return ran;
}

bool ChainRunner::writeReportToFile(const std::string& path) const {
    std::ofstream out(path.c_str(), std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "{\n  \"reports\": [\n";
    for (size_t i = 0; i < mReports.size(); ++i) {
        const ChainReport& report = mReports[i];
        out << "    {\n";
        out << "      \"stage\": \"" << stageToString(report.stage) << "\",\n";
        out << "      \"result\": \"" << resultToString(report.result) << "\",\n";
        out << "      \"message\": \"";
        for (size_t j = 0; j < report.message.size(); ++j) {
            const char c = report.message[j];
            if (c == '"' || c == '\\') {
                out << '\\' << c;
            } else if (c == '\n') {
                out << "\\n";
            } else {
                out << c;
            }
        }
        out << "\"\n";
        out << "    }";
        if (i + 1 < mReports.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "  ]\n}\n";
    return true;
}

} /* namespace primitives */
} /* namespace PP */
