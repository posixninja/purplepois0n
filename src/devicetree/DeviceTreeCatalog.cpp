/*
 * DeviceTreeCatalog.cpp
 */

#include "devicetree/DeviceTreeCatalog.h"
#include "devicetree/JsonMini.h"
#include "devicetree/RegisterInventory.h"
#include "EnvUtil.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace PP {
namespace devicetree {

namespace {

DeviceTreeCatalog* gGlobalCatalog = nullptr;

std::string readFileToString(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) {
        return std::string();
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

bool writeStringToFile(const std::string& path, const std::string& body) {
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    out << body;
    return out.good();
}

uint64_t jsonNumberAsU64(const JsonValue& value) {
    if (value.type != JsonType::Number || value.numberValue < 0.0) {
        return 0;
    }
    return static_cast<uint64_t>(value.numberValue);
}

std::string jsonStringOrEmpty(const JsonValue* value) {
    if (value == nullptr || value->type != JsonType::String) {
        return std::string();
    }
    return value->stringValue;
}

const JsonValue* objectField(const JsonValue& object, const char* key) {
    if (object.type != JsonType::Object) {
        return nullptr;
    }
    const std::map<std::string, JsonValue>::const_iterator it = object.objectValue.find(key);
    if (it == object.objectValue.end()) {
        return nullptr;
    }
    return &it->second;
}

std::string jsonEscape(const std::string& text) {
    std::ostringstream oss;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '"') {
            oss << "\\\"";
        } else if (c == '\\') {
            oss << "\\\\";
        } else if (c == '\n') {
            oss << "\\n";
        } else {
            oss << c;
        }
    }
    return oss.str();
}

uint64_t parseHexString(const std::string& text) {
    if (text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        return std::strtoull(text.c_str() + 2, nullptr, 16);
    }
    return std::strtoull(text.c_str(), nullptr, 10);
}

DeviceTreeCatalog loadExportedCatalogFromJsonText(const std::string& jsonText) {
    DeviceTreeCatalog catalog;
    JsonValue root;
    std::string parseError;
    if (!jsonParse(jsonText, &root, &parseError)) {
        catalog.error = parseError;
        return catalog;
    }
    const JsonValue* summaryObj = objectField(root, "summary");
    if (summaryObj != nullptr) {
        catalog.summary.model = jsonStringOrEmpty(objectField(*summaryObj, "model"));
        catalog.summary.boardConfig = jsonStringOrEmpty(objectField(*summaryObj, "boardConfig"));
        catalog.summary.productName = jsonStringOrEmpty(objectField(*summaryObj, "productName"));
        catalog.summary.socName = jsonStringOrEmpty(objectField(*summaryObj, "socName"));
        catalog.summary.socGeneration = jsonStringOrEmpty(objectField(*summaryObj, "socGeneration"));
        catalog.summary.deviceType = jsonStringOrEmpty(objectField(*summaryObj, "deviceType"));
        const JsonValue* hasVirt = objectField(*summaryObj, "hasVirtualization");
        if (hasVirt != nullptr && hasVirt->type == JsonType::Number && hasVirt->numberValue >= 1.0) {
            catalog.summary.hasVirtualization = true;
            catalog.summary.pageMonitorPresent = true;
        }
        const JsonValue* pageMon = objectField(*summaryObj, "pageMonitorPresent");
        if (pageMon != nullptr && pageMon->type == JsonType::Number && pageMon->numberValue >= 1.0) {
            catalog.summary.pageMonitorPresent = true;
        }
    }
    const JsonValue* regions = objectField(root, "regions");
    if (regions != nullptr && regions->type == JsonType::Array) {
        for (size_t i = 0; i < regions->arrayValue.size(); ++i) {
            const JsonValue& item = regions->arrayValue[i];
            MmioRegion region;
            region.nodePath = jsonStringOrEmpty(objectField(item, "nodePath"));
            region.compatible = jsonStringOrEmpty(objectField(item, "compatible"));
            region.label = jsonStringOrEmpty(objectField(item, "label"));
            const JsonValue* tag = objectField(item, "tag");
            if (tag != nullptr && tag->type == JsonType::String) {
                region.tag = classifyMmioHint(tag->stringValue);
            }
            const JsonValue* phys = objectField(item, "physAddr");
            const JsonValue* size = objectField(item, "size");
            if (phys != nullptr && phys->type == JsonType::String) {
                region.physAddr = parseHexString(phys->stringValue);
            } else if (phys != nullptr && phys->type == JsonType::Number) {
                region.physAddr = jsonNumberAsU64(*phys);
            }
            if (size != nullptr && size->type == JsonType::String) {
                region.size = parseHexString(size->stringValue);
            } else if (size != nullptr && size->type == JsonType::Number) {
                region.size = jsonNumberAsU64(*size);
            }
            catalog.regions.push_back(region);
        }
    }
    const JsonValue* registerArray = objectField(root, "registers");
    if (registerArray != nullptr && registerArray->type == JsonType::Array) {
        for (size_t i = 0; i < registerArray->arrayValue.size(); ++i) {
            const JsonValue& item = registerArray->arrayValue[i];
            DtRegisterEntry reg;
            reg.nodePath = jsonStringOrEmpty(objectField(item, "nodePath"));
            reg.property = jsonStringOrEmpty(objectField(item, "property"));
            reg.compatible = jsonStringOrEmpty(objectField(item, "compatible"));
            reg.label = jsonStringOrEmpty(objectField(item, "label"));
            reg.detail = jsonStringOrEmpty(objectField(item, "detail"));
            const JsonValue* kind = objectField(item, "kind");
            if (kind != nullptr && kind->type == JsonType::String) {
                reg.tag = classifyMmioHint(kind->stringValue);
            }
            const JsonValue* tag = objectField(item, "tag");
            if (tag != nullptr && tag->type == JsonType::String) {
                reg.tag = classifyMmioHint(tag->stringValue);
            }
            const JsonValue* phys = objectField(item, "physAddr");
            const JsonValue* size = objectField(item, "size");
            const JsonValue* offset = objectField(item, "offset");
            if (phys != nullptr && phys->type == JsonType::String) {
                reg.physAddr = parseHexString(phys->stringValue);
            }
            if (size != nullptr && size->type == JsonType::String) {
                reg.size = parseHexString(size->stringValue);
            }
            if (offset != nullptr && offset->type == JsonType::String) {
                reg.offset = parseHexString(offset->stringValue);
            }
            catalog.registers.push_back(reg);
        }
    }
    catalog.success =
        !catalog.registers.empty() || !catalog.regions.empty() || summaryObj != nullptr;
    if (!catalog.success) {
        catalog.error = "exported catalog missing regions/summary";
    }
    return catalog;
}

} /* anonymous */

DeviceTreeCatalog buildCatalogFromJsonText(const std::string& jsonText, bool includeAllRegions) {
    DeviceTreeCatalog catalog;
    JsonValue root;
    std::string parseError;
    if (!jsonParse(jsonText, &root, &parseError)) {
        catalog.error = parseError;
        return catalog;
    }

    const RegisterInventory inventory = buildRegisterInventory(root);
    applyInventoryToCatalog(inventory, !includeAllRegions, &catalog);
    return catalog;
}

DeviceTreeCatalog buildCatalogFromJsonFile(const std::string& jsonPath, bool includeAllRegions) {
    DeviceTreeCatalog catalog;
    const std::string text = readFileToString(jsonPath);
    if (text.empty()) {
        catalog.error = "failed to read JSON: " + jsonPath;
        return catalog;
    }
    catalog = buildCatalogFromJsonText(text, includeAllRegions);
    catalog.sourcePath = jsonPath;
    return catalog;
}

DeviceTreeCatalog buildCatalogFromPath(const std::string& inputPath, bool includeAllRegions) {
    DeviceTreeCatalog catalog;
    catalog.sourcePath = inputPath;

    if (inputPath.size() >= 5 && inputPath.compare(inputPath.size() - 5, 5, ".json") == 0) {
        return buildCatalogFromJsonFile(inputPath, includeAllRegions);
    }

    const std::string ipsw = ToolRunner::findIpswExecutable();
    if (ipsw.empty()) {
        catalog.error = "ipsw not found — build external/ipsw or set PURPLEPOIS0N_IPSW";
        return catalog;
    }

    std::vector<std::string> argv;
    argv.push_back(ipsw);
    argv.push_back("dtree");
    argv.push_back(inputPath);
    argv.push_back("-j");
    const CommandResult run = ToolRunner::run(argv);
    if (run.exitCode != 0 || run.stdoutText.empty()) {
        const std::string fallback = readFileToString(inputPath);
        if (!fallback.empty() && fallback.find('{') != std::string::npos) {
            catalog = buildCatalogFromJsonText(fallback, includeAllRegions);
            catalog.sourcePath = inputPath;
            return catalog;
        }
        catalog.error = run.stderrText.empty() ? "ipsw dtree failed" : run.stderrText;
        return catalog;
    }

    catalog = buildCatalogFromJsonText(run.stdoutText, includeAllRegions);
    catalog.sourcePath = inputPath;
    return catalog;
}

void logCatalogSummary(const DeviceTreeCatalog& catalog) {
    if (!catalog.success) {
        Logger::error("  [DeviceTree] " + catalog.error);
        return;
    }
    Logger::info("  [DeviceTree] source=" + catalog.sourcePath);
    if (!catalog.summary.model.empty()) {
        Logger::info("  [DeviceTree] model=" + catalog.summary.model +
                     (catalog.summary.boardConfig.empty()
                          ? std::string()
                          : " board=" + catalog.summary.boardConfig));
    }
    if (!catalog.summary.socName.empty()) {
        Logger::info("  [DeviceTree] soc=" + catalog.summary.socName +
                     (catalog.summary.socGeneration.empty()
                          ? std::string()
                          : " gen=" + catalog.summary.socGeneration));
    }
    if (!catalog.summary.deviceType.empty()) {
        Logger::info("  [DeviceTree] arm-io=" + catalog.summary.deviceType);
    }
    Logger::info("  [DeviceTree] nodes=" + std::to_string(catalog.nodes.size()) + " registers=" +
                 std::to_string(catalog.registers.size()) + " mmio regions=" +
                 std::to_string(catalog.regions.size()));
}

void logAgxMmioHints(const DeviceTreeCatalog& catalog) {
    if (!catalog.success) {
        return;
    }
    for (size_t i = 0; i < catalog.registers.size(); ++i) {
        const DtRegisterEntry& reg = catalog.registers[i];
        if (reg.tag != MmioTag::AgxGpu && reg.tag != MmioTag::ArmIo) {
            continue;
        }
        std::ostringstream oss;
        oss << "  [DeviceTree] " << mmioTagToString(reg.tag) << " " << reg.nodePath << " "
            << reg.property;
        if (!reg.label.empty()) {
            oss << " label=" << reg.label;
        }
        if (reg.physAddr != 0 || reg.size != 0) {
            oss << " pa=0x" << std::hex << reg.physAddr;
            if (reg.size != 0) {
                oss << " sz=0x" << reg.size;
            }
            oss << std::dec;
        }
        if (reg.offset != 0) {
            oss << " off=0x" << std::hex << reg.offset << std::dec;
        }
        if (!reg.compatible.empty()) {
            oss << " compat=" << reg.compatible;
        }
        Logger::info(oss.str());
    }
}

void logFullRegisterInventory(const DeviceTreeCatalog& catalog, size_t maxEntries) {
    if (!catalog.success) {
        return;
    }
    RegisterInventory inventory;
    inventory.summary = catalog.summary;
    inventory.nodes = catalog.nodes;
    inventory.registers = catalog.registers;
    logRegisterInventorySummary(inventory);
    logRegisterInventoryVerbose(inventory, maxEntries);
}

bool writeCatalogJson(const DeviceTreeCatalog& catalog, const std::string& outPath, std::string* error) {
    if (!catalog.success) {
        if (error != nullptr) {
            *error = catalog.error.empty() ? "catalog build failed" : catalog.error;
        }
        return false;
    }

    std::ostringstream body;
    body << "{\n";
    body << "  \"source\": \"" << jsonEscape(catalog.sourcePath) << "\",\n";
    body << "  \"summary\": {\n";
    body << "    \"model\": \"" << jsonEscape(catalog.summary.model) << "\",\n";
    body << "    \"boardConfig\": \"" << jsonEscape(catalog.summary.boardConfig) << "\",\n";
    body << "    \"productName\": \"" << jsonEscape(catalog.summary.productName) << "\",\n";
    body << "    \"socName\": \"" << jsonEscape(catalog.summary.socName) << "\",\n";
    body << "    \"socGeneration\": \"" << jsonEscape(catalog.summary.socGeneration) << "\",\n";
    body << "    \"deviceType\": \"" << jsonEscape(catalog.summary.deviceType) << "\",\n";
    body << "    \"hasVirtualization\": " << (catalog.summary.hasVirtualization ? "true" : "false")
         << ",\n";
    body << "    \"pageMonitorPresent\": " << (catalog.summary.pageMonitorPresent ? "true" : "false")
         << "\n";
    body << "  },\n";
    body << "  \"registers\": [\n";
    for (size_t i = 0; i < catalog.registers.size(); ++i) {
        const DtRegisterEntry& reg = catalog.registers[i];
        body << "    {\n";
        body << "      \"nodePath\": \"" << jsonEscape(reg.nodePath) << "\",\n";
        body << "      \"property\": \"" << jsonEscape(reg.property) << "\",\n";
        body << "      \"compatible\": \"" << jsonEscape(reg.compatible) << "\",\n";
        body << "      \"kind\": \"" << registerKindToString(reg.kind) << "\",\n";
        body << "      \"tag\": \"" << mmioTagToString(reg.tag) << "\",\n";
        body << "      \"label\": \"" << jsonEscape(reg.label) << "\",\n";
        body << "      \"detail\": \"" << jsonEscape(reg.detail) << "\",\n";
        body << "      \"index\": " << reg.index << ",\n";
        body << "      \"physAddr\": \"0x" << std::hex << reg.physAddr << "\",\n";
        body << "      \"size\": \"0x" << reg.size << "\",\n";
        body << "      \"offset\": \"0x" << reg.offset << "\",\n";
        body << "      \"end\": \"0x" << reg.end << "\",\n";
        body << "      \"flags\": " << std::dec << reg.flags << ",\n";
        body << "      \"regBank\": " << reg.regBank << ",\n";
        body << "      \"unk\": " << reg.unk << ",\n";
        body << "      \"phandle\": " << reg.phandle << "\n";
        body << "    }";
        if (i + 1 < catalog.registers.size()) {
            body << ",";
        }
        body << "\n";
    }
    body << "  ],\n";
    body << "  \"regions\": [\n";
    for (size_t i = 0; i < catalog.regions.size(); ++i) {
        const MmioRegion& region = catalog.regions[i];
        body << "    {\n";
        body << "      \"nodePath\": \"" << jsonEscape(region.nodePath) << "\",\n";
        body << "      \"compatible\": \"" << jsonEscape(region.compatible) << "\",\n";
        body << "      \"label\": \"" << jsonEscape(region.label) << "\",\n";
        body << "      \"tag\": \"" << mmioTagToString(region.tag) << "\",\n";
        body << "      \"physAddr\": \"0x" << std::hex << region.physAddr << "\",\n";
        body << "      \"size\": \"0x" << region.size << std::dec << "\"\n";
        body << "    }";
        if (i + 1 < catalog.regions.size()) {
            body << ",";
        }
        body << "\n";
    }
    body << "  ]\n";
    body << "}\n";

    if (!writeStringToFile(outPath, body.str())) {
        if (error != nullptr) {
            *error = "failed to write " + outPath;
        }
        return false;
    }
    return true;
}

const DeviceTreeCatalog* globalCatalog() { return gGlobalCatalog; }

void setGlobalCatalog(const DeviceTreeCatalog& catalog) {
    static DeviceTreeCatalog storage;
    storage = catalog;
    gGlobalCatalog = catalog.success ? &storage : nullptr;
}

bool loadGlobalCatalogFromEnv() {
    const std::string path = envOrEmpty("PURPLEPOIS0N_MMIO_CATALOG");
    if (path.empty()) {
        return false;
    }
    const std::string text = readFileToString(path);
    if (text.empty()) {
        Logger::warn("  [DeviceTree] PURPLEPOIS0N_MMIO_CATALOG unreadable: " + path);
        return false;
    }
    DeviceTreeCatalog catalog = loadExportedCatalogFromJsonText(text);
    if (!catalog.success) {
        catalog = buildCatalogFromJsonText(text, false);
    }
    if (!catalog.success) {
        Logger::warn("  [DeviceTree] failed to load PURPLEPOIS0N_MMIO_CATALOG: " + catalog.error);
        return false;
    }
    catalog.sourcePath = path;
    setGlobalCatalog(catalog);
    return true;
}

} /* namespace devicetree */
} /* namespace PP */
