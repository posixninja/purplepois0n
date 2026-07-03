/*
 * ChainRunner.cpp
 */

#include "primitives/ChainRunner.h"
#include "primitives/pongo/PongoChain.h"
#include "primitives/PongoDelegate.h"
#include "EnvUtil.h"
#include "primitives/PrimitiveRegistry.h"
#include "primitives/ExploitModulePrimitive.h"
#include "primitives/ExploitDelegate.h"
#include "primitives/Gen6Types.h"
#include "primitives/ITransport.h"
#include "Gen0Workflow.h"
#include "RamdiskWorkDir.h"
#include "Checkm8.h"
#include "Logger.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

namespace PP {
namespace primitives {

namespace {

constexpr const char* kSupportDoc = "docs/SUPPORT.md";

const ChainStage kGen6Stages[] = {
    ChainStage::Kernelcache,
    ChainStage::Patchfind,
    ChainStage::KernelExploit,
    ChainStage::PacBypass,
    ChainStage::PplBypass,
    ChainStage::PageMonitor,
    ChainStage::PhysRw,
    ChainStage::Privilege,
    ChainStage::TrustCache,
    ChainStage::Bootstrap,
};

const ChainStage kClassicStages[] = {
    ChainStage::Kernelcache,
    ChainStage::Patchfind,
    ChainStage::KernelExploit,
    ChainStage::Privilege,
    ChainStage::Bootstrap,
};

bool exploitModuleMatchesEra(const ExploitModulePrimitive* module, JailbreakGeneration era) {
    const JailbreakGeneration scope = module->eraScope();
    if (era == JailbreakGeneration::Unknown) {
        return scope == JailbreakGeneration::Gen6;
    }
    return scope == era;
}

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

struct ModulePriority {
    ExploitModulePrimitive* module;
    uint64_t priority;
};

bool modulePriorityGreater(const ModulePriority& a, const ModulePriority& b) {
    return a.priority > b.priority;
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
        if (primitive->gen6Stage() != ChainStage::Probe) {
            continue;
        }
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
        if (context.recoveryChainRun &&
            std::string(primitive->name()) == "recovery-boot-chain") {
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

bool ChainRunner::runOrderedChain(ExecutionContext& context,
                                  bool executeMode,
                                  const ChainStage* stages,
                                  size_t stageCount,
                                  JailbreakGeneration eraFilter) {
    PrimitiveRegistry& registry = PrimitiveRegistry::instance();
    registry.registerBuiltins();
    const std::vector<Primitive*> primitives = registry.list();

    for (size_t s = 0; s < stageCount; ++s) {
        const ChainStage chainStage = stages[s];
        logStage(chainStage, std::string("era chain stage (") +
                                  jailbreakGenerationToString(eraFilter) + ")");

        std::vector<ModulePriority> exploitModules;
        bool anyRan = false;

        for (size_t i = 0; i < primitives.size(); ++i) {
            Primitive* primitive = primitives[i];
            if (primitive->gen6Stage() != chainStage) {
                continue;
            }
            if (!stateAllowed(primitive->requiredDeviceStates(), context.deviceState)) {
                continue;
            }
            if (!primitive->canRun(context)) {
                continue;
            }

            ExploitModulePrimitive* exploitModule = dynamic_cast<ExploitModulePrimitive*>(primitive);
            if (exploitModule != nullptr) {
                if (!exploitModuleMatchesEra(exploitModule, eraFilter)) {
                    continue;
                }
                ModulePriority entry;
                entry.module = exploitModule;
                entry.priority = exploitModule->priority();
                exploitModules.push_back(entry);
                continue;
            }

            const bool savedMutation = context.allowMutation;
            context.allowMutation = executeMode;

            std::ostringstream prefix;
            prefix << primitive->name() << " (" << categoryToString(primitive->category()) << "): ";
            logStage(chainStage, prefix.str() + "running");

            const PrimitiveResult result = primitive->execute(context);
            context.allowMutation = savedMutation;

            recordReport(chainStage, result, prefix.str() + resultToString(result));
            anyRan = true;
        }

        if (!exploitModules.empty()) {
            std::sort(exploitModules.begin(), exploitModules.end(), modulePriorityGreater);

            if (executeMode) {
                bool picked = false;
                for (size_t k = 0; k < exploitModules.size(); ++k) {
                    ExploitModulePrimitive* module = exploitModules[k].module;
                    if (!module->contextSupported(context)) {
                        continue;
                    }

                    const bool savedMutation = context.allowMutation;
                    context.allowMutation = executeMode;

                    std::ostringstream prefix;
                    prefix << module->name() << " (" << categoryToString(module->category()) << "): ";
                    logStage(chainStage, prefix.str() + "running");

                    const PrimitiveResult result = module->execute(context);
                    context.allowMutation = savedMutation;

                    recordReport(chainStage, result, prefix.str() + resultToString(result));
                    anyRan = true;
                    picked = true;
                    break;
                }
                if (!picked) {
                    recordReport(chainStage, PrimitiveResult::NotApplicable,
                                 "no exploit module supports this device profile");
                }
            } else {
                for (size_t k = 0; k < exploitModules.size(); ++k) {
                    ExploitModulePrimitive* module = exploitModules[k].module;

                    const bool savedMutation = context.allowMutation;
                    context.allowMutation = executeMode;

                    std::ostringstream prefix;
                    prefix << module->name() << " (" << categoryToString(module->category()) << "): ";
                    logStage(chainStage, prefix.str() + "running");

                    const PrimitiveResult result = module->execute(context);
                    context.allowMutation = savedMutation;

                    recordReport(chainStage, result, prefix.str() + resultToString(result));
                    anyRan = true;
                }
            }
        }

        if (!anyRan) {
            recordReport(chainStage, PrimitiveResult::NotApplicable, "no primitives ran");
        }
    }

    return true;
}

bool ChainRunner::runRecoveryMiniChain(ExecutionContext& context, bool executeMode) {
    if (context.recoveryChain.empty() && context.recoveryChainRun && !context.ipswPath.empty()) {
        const std::string workDir = resolveRamdiskWorkDir(context.ramdiskWorkDir);
        populateDefaultRecoveryChain(context.ipswPath, workDir, &context.recoveryChain);
    }

    PrimitiveRegistry& registry = PrimitiveRegistry::instance();
    registry.registerBuiltins();
    Primitive* primitive = registry.findByName("recovery-boot-chain");
    if (primitive == nullptr) {
        logStage(ChainStage::Probe, "Recovery: recovery-boot-chain primitive missing");
        return false;
    }
    if (!primitive->canRun(context)) {
        logStage(ChainStage::Probe, "Recovery: chain prerequisites missing (--recovery-chain --ipsw)");
        recordReport(ChainStage::Probe, PrimitiveResult::PrerequisitesMissing,
                     "recovery-boot-chain");
        return false;
    }

    const bool savedMutation = context.allowMutation;
    context.allowMutation = executeMode;
    logStage(executeMode ? ChainStage::Execute : ChainStage::Probe,
             "Recovery: iBSS → iBEC → rdsk via recovery-boot-chain");
    const PrimitiveResult result = primitive->execute(context);
    context.allowMutation = savedMutation;
    recordReport(executeMode ? ChainStage::Execute : ChainStage::Probe, result,
                 "RecoveryBootChainPrimitive");
    return result == PrimitiveResult::Success;
}

bool ChainRunner::runPongoMiniChain(ExecutionContext& context, bool executeMode) {
    return runPongoChain(context, executeMode, *this);
}

bool ChainRunner::runGen5DfuMiniChain(ExecutionContext& context) {
    logStage(ChainStage::Probe, "Gen5 DFU: checkm8 bootrom entry (see Checkm8BootromPrimitive)");
    recordReport(ChainStage::Probe, PrimitiveResult::Success, "Gen5 checkm8 stage");
    if (!context.ipswPath.empty() || context.recoveryChainRun || !context.recoveryChain.empty()) {
        logStage(ChainStage::Probe,
                 "Gen5 DFU → Recovery: use RecoveryBootChainPrimitive after mode transition");
        recordReport(ChainStage::Probe, PrimitiveResult::Success, "see recovery-boot-chain");
    } else {
        logStage(ChainStage::Probe, "Gen5 DFU: signed iBSS — use Recovery + recovery-upload");
        recordReport(ChainStage::Probe, PrimitiveResult::Success, "see RecoveryUploadPrimitive");
    }
    logStage(ChainStage::Probe, "Gen5 DFU: bootstrap — delegate checkra1n/palera1n");
    recordReport(ChainStage::Probe, PrimitiveResult::Success, "bootstrap delegate only");
    if (context.pongoProbeRun || context.pongoBootRun) {
        if (context.allowMutation && exploitPluginsEnabled() &&
            (context.pongoSpawnCheckra1n || context.pongoBootRun)) {
            const PrimitiveResult spawn = PongoDelegate::spawnCheckra1nShell(true);
            if (spawn != PrimitiveResult::Success) {
                recordReport(ChainStage::Execute, spawn, "PongoDelegate spawnCheckra1nShell");
                return false;
            }
        }
        if (!runPongoMiniChain(context, context.allowMutation)) {
            return false;
        }
    }
    return true;
}

bool ChainRunner::runEraChain(ExecutionContext& context, bool executeMode) {
    if (context.jailbreakGeneration == JailbreakGeneration::Unknown) {
        context.jailbreakGeneration = detectJailbreakGeneration(context);
    }

    const JailbreakGeneration era = context.jailbreakGeneration;

    if (context.deviceState == DeviceState::DFU && era == JailbreakGeneration::Gen5) {
        if (!runGen5DfuMiniChain(context)) {
            return false;
        }
        return runOrderedChain(context, executeMode, kClassicStages,
                               sizeof(kClassicStages) / sizeof(kClassicStages[0]),
                               JailbreakGeneration::Gen5);
    }

    if (context.deviceState == DeviceState::Normal ||
        context.deviceState == DeviceState::Unknown) {
        if (era == JailbreakGeneration::Gen6 || era == JailbreakGeneration::Unknown) {
            return runOrderedChain(context, executeMode, kGen6Stages,
                                   sizeof(kGen6Stages) / sizeof(kGen6Stages[0]),
                                   JailbreakGeneration::Gen6);
        }
        if (era == JailbreakGeneration::Gen1to4 || era == JailbreakGeneration::Gen0) {
            return runOrderedChain(context, executeMode, kClassicStages,
                                   sizeof(kClassicStages) / sizeof(kClassicStages[0]), era);
        }
    }

    if (context.deviceState == DeviceState::DFU && era == JailbreakGeneration::Gen0) {
        return runOrderedChain(context, executeMode, kClassicStages,
                               sizeof(kClassicStages) / sizeof(kClassicStages[0]),
                               JailbreakGeneration::Gen0);
    }

    if (context.deviceState == DeviceState::Recovery) {
        return runRecoveryMiniChain(context, executeMode);
    }

    return false;
}

bool ChainRunner::runProbeChain(ExecutionContext& context) {
    mReports.clear();

    if (context.jailbreakGeneration == JailbreakGeneration::Unknown) {
        context.jailbreakGeneration = detectJailbreakGeneration(context);
    }

    logStage(ChainStage::Detect, std::string("device mode: ") + deviceStateLabel(context.deviceState));
    if (context.cpid != 0) {
        std::ostringstream oss;
        oss << "CPID 0x" << std::hex << context.cpid << std::dec << " ("
            << Checkm8::cpidToSocName(context.cpid) << ")";
        logStage(ChainStage::Detect, oss.str());
    }
    if (!context.iosVersion.empty()) {
        logStage(ChainStage::Detect, "iOS " + context.iosVersion);
    }
    logStage(ChainStage::Detect,
             std::string("jailbreak era: ") +
                 jailbreakGenerationToString(context.jailbreakGeneration));
    recordReport(ChainStage::Detect, PrimitiveResult::Success,
                 std::string("era ") +
                     jailbreakGenerationToString(context.jailbreakGeneration));

    if (context.transport != nullptr) {
        logStage(ChainStage::Connect,
                 std::string("transport: ") + context.transport->transportName());
        recordReport(ChainStage::Connect, PrimitiveResult::Success, "transport attached");
    } else {
        logStage(ChainStage::Connect, "no live transport (offline probes only)");
        recordReport(ChainStage::Connect, PrimitiveResult::NotApplicable, "no transport");
    }

    context.allowMutation = false;

    if (context.deviceState == DeviceState::Normal || context.deviceState == DeviceState::DFU ||
        context.deviceState == DeviceState::Recovery) {
        runEraChain(context, false);
    }

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
    bool eraSuccess = true;
    if (context.deviceState == DeviceState::Normal || context.deviceState == DeviceState::DFU ||
        context.deviceState == DeviceState::Recovery) {
        eraSuccess = runEraChain(context, true);
    }
    if (!eraSuccess) {
        ExploitDelegate::cleanupAll();
        recordReport(ChainStage::Execute, PrimitiveResult::Failed, "era execute chain failed");
        return false;
    }
    const bool ran = runStage(ChainStage::Execute, context, true);
    ExploitDelegate::cleanupAll();
    recordReport(ChainStage::Execute,
                 ran ? PrimitiveResult::Success : PrimitiveResult::NotApplicable,
                 "execute chain finished");
    return true;
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
