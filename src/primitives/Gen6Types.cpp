/*
 * Gen6Types.cpp
 */

#include "primitives/Gen6Types.h"

#include "Checkm8.h"

#include <cstdlib>

namespace PP {
namespace primitives {

namespace {

int compareVersionComponents(const std::string& a, const std::string& b) {
    size_t i = 0;
    size_t j = 0;
    while (i < a.size() || j < b.size()) {
        long long va = 0;
        long long vb = 0;
        while (i < a.size() && a[i] != '.') {
            va = va * 10 + (a[i] - '0');
            ++i;
        }
        while (j < b.size() && b[j] != '.') {
            vb = vb * 10 + (b[j] - '0');
            ++j;
        }
        if (va < vb) {
            return -1;
        }
        if (va > vb) {
            return 1;
        }
        if (i < a.size()) {
            ++i;
        }
        if (j < b.size()) {
            ++j;
        }
    }
    return 0;
}

} /* anonymous namespace */

const char* exploitModuleKindToString(ExploitModuleKind kind) {
    switch (kind) {
        case ExploitModuleKind::Kernel:
            return "Kernel";
        case ExploitModuleKind::PacBypass:
            return "PacBypass";
        case ExploitModuleKind::PplBypass:
            return "PplBypass";
    }
    return "Unknown";
}

const char* exploitModuleIdToString(ExploitModuleId id) {
    switch (id) {
        case ExploitModuleId::Kfd:
            return "kfd";
        case ExploitModuleId::WeightBufs:
            return "weightBufs";
        case ExploitModuleId::MulticastBytecopy:
            return "multicast_bytecopy";
        case ExploitModuleId::DarkSword:
            return "DarkSword";
        case ExploitModuleId::BadRecovery:
            return "badRecovery";
        case ExploitModuleId::DmaFail:
            return "dmaFail";
        case ExploitModuleId::Limera1n:
            return "limera1n";
        case ExploitModuleId::Evasi0n:
            return "evasi0n";
        case ExploitModuleId::Checkra1n:
            return "checkra1n";
        case ExploitModuleId::Kpwn24k:
            return "24kpwn";
    }
    return "unknown";
}

const char* jailbreakGenerationToString(JailbreakGeneration generation) {
    switch (generation) {
        case JailbreakGeneration::Gen0:
            return "Gen0";
        case JailbreakGeneration::Gen1to4:
            return "Gen1-4";
        case JailbreakGeneration::Gen5:
            return "Gen5";
        case JailbreakGeneration::Gen6:
            return "Gen6";
        case JailbreakGeneration::Unknown:
            return "Unknown";
    }
    return "Unknown";
}

const char* exploitModuleEnvKey(ExploitModuleId id) {
    switch (id) {
        case ExploitModuleId::Kfd:
            return "PURPLEPOIS0N_DOPAMINE_KFD";
        case ExploitModuleId::WeightBufs:
            return "PURPLEPOIS0N_DOPAMINE_WEIGHTBUFS";
        case ExploitModuleId::MulticastBytecopy:
            return "PURPLEPOIS0N_DOPAMINE_MULTICAST_BYTECOPY";
        case ExploitModuleId::DarkSword:
            return "PURPLEPOIS0N_DOPAMINE_DARKSWORD";
        case ExploitModuleId::BadRecovery:
            return "PURPLEPOIS0N_DOPAMINE_BADRECOVERY";
        case ExploitModuleId::DmaFail:
            return "PURPLEPOIS0N_DOPAMINE_DMAFAIL";
        case ExploitModuleId::Limera1n:
            return "PURPLEPOIS0N_LIMERA1N";
        case ExploitModuleId::Evasi0n:
            return "PURPLEPOIS0N_EVASI0N";
        case ExploitModuleId::Checkra1n:
            return "PURPLEPOIS0N_CHECKRA1N";
        case ExploitModuleId::Kpwn24k:
            return "PURPLEPOIS0N_24KPWN";
    }
    return "PURPLEPOIS0N_EXPLOIT_MODULE";
}

JailbreakGeneration detectJailbreakGeneration(const ExecutionContext& context) {
    if (context.deviceState == DeviceState::DFU) {
        if (context.cpid != 0 && Checkm8::isSupportedCpid(context.cpid)) {
            return JailbreakGeneration::Gen5;
        }
        return JailbreakGeneration::Gen0;
    }
    if (context.deviceState == DeviceState::Recovery) {
        return JailbreakGeneration::Gen0;
    }
    if (!context.iosVersion.empty()) {
        if (iosVersionInRange(context.iosVersion, "4.0", "5.9")) {
            return JailbreakGeneration::Gen0;
        }
        if (iosVersionInRange(context.iosVersion, "6.0", "14.9")) {
            return JailbreakGeneration::Gen1to4;
        }
        if (iosVersionInRange(context.iosVersion, "15.0", "99.0")) {
            return JailbreakGeneration::Gen6;
        }
    }
    return JailbreakGeneration::Unknown;
}

ChainStage gen6StageForCategory(PrimitiveCategory category) {
    switch (category) {
        case PrimitiveCategory::Patchfinding:
            return ChainStage::Patchfind;
        case PrimitiveCategory::Kernel:
            return ChainStage::KernelExploit;
        case PrimitiveCategory::PacBypass:
            return ChainStage::PacBypass;
        case PrimitiveCategory::PplBypass:
            return ChainStage::PplBypass;
        case PrimitiveCategory::PageMonitor:
            return ChainStage::PageMonitor;
        case PrimitiveCategory::PhysRw:
            return ChainStage::PhysRw;
        case PrimitiveCategory::Privilege:
            return ChainStage::Privilege;
        case PrimitiveCategory::TrustCache:
            return ChainStage::TrustCache;
        case PrimitiveCategory::Bootstrap:
            return ChainStage::Bootstrap;
        default:
            return ChainStage::Probe;
    }
}

bool isHistoricalDelegateId(ExploitModuleId id) {
    return id == ExploitModuleId::Limera1n || id == ExploitModuleId::Evasi0n ||
           id == ExploitModuleId::Checkra1n || id == ExploitModuleId::Kpwn24k;
}

bool iosVersionInRange(const std::string& version, const char* start, const char* end) {
    if (version.empty()) {
        return false;
    }
    const int startCmp = compareVersionComponents(version, start);
    const int endCmp = compareVersionComponents(version, end);
    return (startCmp >= 0) && (endCmp <= 0);
}

} /* namespace primitives */
} /* namespace PP */
