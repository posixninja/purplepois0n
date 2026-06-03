/*
 * PrimitiveTypes.h
 *
 * Core enums and execution context for the primitive taxonomy framework.
 */

#ifndef PRIMITIVES_PRIMITIVE_TYPES_H_
#define PRIMITIVES_PRIMITIVE_TYPES_H_

#include "../DeviceState.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PP {
namespace primitives {

class ITransport;

/** Security domain a primitive targets. */
enum class PrimitiveCategory {
    Bootrom,
    Kernel,
    Codesign,
    Sandbox,
    Injection
};

/** Action a primitive can perform (capability flags). */
enum class PrimitiveOperation {
    Read,
    Write,
    Overwrite,
    Patch,
    Inject,
    Execute,
    Probe
};

/** Outcome of a primitive execution attempt. */
enum class PrimitiveResult {
    Success,
    NotApplicable,
    Unsupported,
    PrerequisitesMissing,
    TransportError,
    ProbeOnly,
    PluginDisabled,
    Failed
};

/** Workflow stage labels for ChainRunner output. */
enum class ChainStage {
    Detect,
    Connect,
    Probe,
    Report,
    Execute
};

/** Per-run context passed to every primitive. */
struct ExecutionContext {
    DeviceState deviceState = DeviceState::Unknown;
    uint32_t cpid = 0;
    uint64_t ecid = 0;
    std::string udid;
    bool allowMutation = false;
    ITransport* transport = nullptr;
};

/** Returns true when built with PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS. */
bool exploitPluginsEnabled();

/** Human-readable names for logging and reports. */
const char* categoryToString(PrimitiveCategory category);
const char* operationToString(PrimitiveOperation operation);
const char* resultToString(PrimitiveResult result);
const char* stageToString(ChainStage stage);

/** Test whether @p ops contains @p op. */
bool supportsOperation(const std::vector<PrimitiveOperation>& ops, PrimitiveOperation op);

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_PRIMITIVE_TYPES_H_ */
