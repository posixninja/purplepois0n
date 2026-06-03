/*
 * PrimitiveTypes.cpp
 */

#include "primitives/PrimitiveTypes.h"

namespace PP {
namespace primitives {

bool exploitPluginsEnabled() {
#if defined(PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS)
    return true;
#else
    return false;
#endif
}

const char* categoryToString(PrimitiveCategory category) {
    switch (category) {
        case PrimitiveCategory::Bootrom:
            return "Bootrom";
        case PrimitiveCategory::Kernel:
            return "Kernel";
        case PrimitiveCategory::Codesign:
            return "Codesign";
        case PrimitiveCategory::Sandbox:
            return "Sandbox";
        case PrimitiveCategory::Injection:
            return "Injection";
    }
    return "Unknown";
}

const char* operationToString(PrimitiveOperation operation) {
    switch (operation) {
        case PrimitiveOperation::Read:
            return "Read";
        case PrimitiveOperation::Write:
            return "Write";
        case PrimitiveOperation::Overwrite:
            return "Overwrite";
        case PrimitiveOperation::Patch:
            return "Patch";
        case PrimitiveOperation::Inject:
            return "Inject";
        case PrimitiveOperation::Execute:
            return "Execute";
        case PrimitiveOperation::Probe:
            return "Probe";
    }
    return "Unknown";
}

const char* resultToString(PrimitiveResult result) {
    switch (result) {
        case PrimitiveResult::Success:
            return "Success";
        case PrimitiveResult::NotApplicable:
            return "NotApplicable";
        case PrimitiveResult::Unsupported:
            return "Unsupported";
        case PrimitiveResult::PrerequisitesMissing:
            return "PrerequisitesMissing";
        case PrimitiveResult::TransportError:
            return "TransportError";
        case PrimitiveResult::ProbeOnly:
            return "ProbeOnly";
        case PrimitiveResult::PluginDisabled:
            return "PluginDisabled";
        case PrimitiveResult::Failed:
            return "Failed";
    }
    return "Unknown";
}

const char* stageToString(ChainStage stage) {
    switch (stage) {
        case ChainStage::Detect:
            return "Detect";
        case ChainStage::Connect:
            return "Connect";
        case ChainStage::Probe:
            return "Probe";
        case ChainStage::Report:
            return "Report";
        case ChainStage::Execute:
            return "Execute";
    }
    return "Unknown";
}

bool supportsOperation(const std::vector<PrimitiveOperation>& ops, PrimitiveOperation op) {
    for (size_t i = 0; i < ops.size(); ++i) {
        if (ops[i] == op) {
            return true;
        }
    }
    return false;
}

} /* namespace primitives */
} /* namespace PP */
