/*
 * Gen6Types.h
 *
 * Dopamine 2.x-aligned exploit module IDs and chain staging (Generation 6).
 */

#ifndef PRIMITIVES_GEN6_TYPES_H_
#define PRIMITIVES_GEN6_TYPES_H_

#include "PrimitiveTypes.h"

namespace PP {
namespace primitives {

/** Dopamine DOExploitType-aligned module classes. */
enum class ExploitModuleKind {
    Kernel,
    PacBypass,
    PplBypass
};

/** Known integrated exploit modules (picker identifiers). */
enum class ExploitModuleId {
    Kfd,
    WeightBufs,
    MulticastBytecopy,
    DarkSword,
    BadRecovery,
    DmaFail,
    Limera1n,
    Evasi0n,
    Checkra1n,
    Kpwn24k
};

const char* exploitModuleKindToString(ExploitModuleKind kind);
const char* exploitModuleIdToString(ExploitModuleId id);
const char* jailbreakGenerationToString(JailbreakGeneration generation);

/** Infer jailbreak era from device state, CPID, and iOS version. */
JailbreakGeneration detectJailbreakGeneration(const ExecutionContext& context);

/** Env var for direct path to exploit binary / tool. */
const char* exploitModuleEnvKey(ExploitModuleId id);

/** Gen 0–5 historical delegates (no Dopamine framework path). */
bool isHistoricalDelegateId(ExploitModuleId id);

/** Chain stage used by Gen6 primitives (Dopamine DOJailbreaker order). */
ChainStage gen6StageForCategory(PrimitiveCategory category);

/** True when @p version is within [@p start, @p end] (NSString-style numeric compare). */
bool iosVersionInRange(const std::string& version, const char* start, const char* end);

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_GEN6_TYPES_H_ */
