/*
 * RegisterInventory.cpp
 */

#include "devicetree/RegisterInventory.h"
#include "Logger.h"

#include <iomanip>
#include <map>
#include <sstream>

namespace PP {
namespace devicetree {

namespace {

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

const JsonValue* objectFieldAny(const JsonValue& object, const char* lower, const char* upper) {
    const JsonValue* value = objectField(object, lower);
    if (value != nullptr) {
        return value;
    }
    return objectField(object, upper);
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

std::string joinPath(const std::string& base, const std::string& leaf) {
    if (base.empty()) {
        return leaf;
    }
    if (leaf.empty()) {
        return base;
    }
    return base + "/" + leaf;
}

std::string compatibleFromProps(const JsonValue& props) {
    const JsonValue* compatibleField = objectField(props, "compatible");
    if (compatibleField == nullptr) {
        return std::string();
    }
    if (compatibleField->type == JsonType::String) {
        return compatibleField->stringValue;
    }
    if (compatibleField->type == JsonType::Array && !compatibleField->arrayValue.empty()) {
        return jsonStringOrEmpty(&compatibleField->arrayValue[0]);
    }
    return std::string();
}

void extractSummaryFields(const JsonValue& object, DeviceTreeSummary* summary) {
    if (object.type != JsonType::Object) {
        return;
    }
    const JsonValue* model = objectField(object, "model");
    if (model != nullptr && model->type == JsonType::String) {
        summary->model = model->stringValue;
    }
    const JsonValue* compatible = objectField(object, "compatible");
    if (compatible != nullptr) {
        if (compatible->type == JsonType::String) {
            summary->boardConfig = compatible->stringValue;
        } else if (compatible->type == JsonType::Array) {
            for (size_t i = 0; i < compatible->arrayValue.size(); ++i) {
                const std::string item = jsonStringOrEmpty(&compatible->arrayValue[i]);
                if (!item.empty() && item.find("Apple") == std::string::npos &&
                    item.find(summary->model) == std::string::npos) {
                    summary->boardConfig = item;
                    break;
                }
            }
        }
    }
    const JsonValue* productName = objectField(object, "product-name");
    if (productName != nullptr) {
        summary->productName = jsonStringOrEmpty(productName);
    }
    const JsonValue* socName = objectField(object, "product-soc-name");
    if (socName != nullptr) {
        summary->socName = jsonStringOrEmpty(socName);
    }
    const JsonValue* socGeneration = objectField(object, "soc-generation");
    if (socGeneration != nullptr) {
        summary->socGeneration = jsonStringOrEmpty(socGeneration);
    }
    const JsonValue* hasVirt = objectField(object, "has-virtualization");
    if (hasVirt != nullptr && hasVirt->type == JsonType::Number && hasVirt->numberValue >= 1.0) {
        summary->hasVirtualization = true;
        summary->pageMonitorPresent = true;
    }
    const JsonValue* compatibleArm = objectField(object, "compatible");
    if (compatibleArm != nullptr && compatibleArm->type == JsonType::String) {
        std::string value = compatibleArm->stringValue;
        if (value.find("arm-io,") == 0) {
            summary->deviceType = value.substr(7);
        }
    }
}

DtRegisterEntry makeEntry(const std::string& nodePath, const std::string& property,
                          const std::string& compatible, RegisterKind kind, size_t index) {
    DtRegisterEntry entry;
    entry.nodePath = nodePath;
    entry.property = property;
    entry.compatible = compatible;
    entry.kind = kind;
    entry.index = index;
    entry.tag = classifyMmioHint(nodePath + " " + compatible + " " + property);
    return entry;
}

void pushRegister(DtRegisterEntry entry, RegisterInventory* inventory) {
    if (!entry.label.empty()) {
        entry.tag = classifyMmioHint(entry.nodePath + " " + entry.compatible + " " + entry.label);
    }
    inventory->registers.push_back(entry);
}

void extractRegProperty(const JsonValue& value, const std::string& nodePath, const std::string& compatible,
                        RegisterInventory* inventory) {
    if (value.type == JsonType::Number) {
        DtRegisterEntry entry =
            makeEntry(nodePath, "reg", compatible, RegisterKind::ScalarMmio, 0);
        entry.physAddr = jsonNumberAsU64(value);
        pushRegister(entry, inventory);
        return;
    }
    if (value.type == JsonType::Object) {
        const JsonValue* addr = objectFieldAny(value, "addr", "Addr");
        const JsonValue* sz = objectFieldAny(value, "size", "Size");
        if (addr != nullptr && sz != nullptr) {
            DtRegisterEntry entry = makeEntry(nodePath, "reg", compatible, RegisterKind::PmgrWindow, 0);
            entry.physAddr = jsonNumberAsU64(*addr);
            entry.size = jsonNumberAsU64(*sz);
            pushRegister(entry, inventory);
        }
        return;
    }
    if (value.type == JsonType::Array) {
        if (value.arrayValue.size() >= 2 &&
            value.arrayValue[0].type == JsonType::Object) {
            for (size_t i = 0; i < value.arrayValue.size(); ++i) {
                extractRegProperty(value.arrayValue[i], nodePath, compatible, inventory);
            }
            return;
        }
        for (size_t i = 0; i + 1 < value.arrayValue.size(); i += 2) {
            DtRegisterEntry entry =
                makeEntry(nodePath, "reg", compatible, RegisterKind::Reg, i / 2);
            entry.physAddr = jsonNumberAsU64(value.arrayValue[i]);
            entry.size = jsonNumberAsU64(value.arrayValue[i + 1]);
            pushRegister(entry, inventory);
        }
    }
}

void extractKnownProperty(const std::string& key, const JsonValue& value, const std::string& nodePath,
                          const std::string& compatible, RegisterInventory* inventory) {
    if (key == "reg") {
        extractRegProperty(value, nodePath, compatible, inventory);
        return;
    }
    if (key == "reg-private") {
        if (value.type == JsonType::Number) {
            DtRegisterEntry entry =
                makeEntry(nodePath, key, compatible, RegisterKind::RegPrivate, 0);
            entry.physAddr = jsonNumberAsU64(value);
            pushRegister(entry, inventory);
        }
        return;
    }
    if (key == "pmap-io-ranges" && value.type == JsonType::Array) {
        for (size_t i = 0; i < value.arrayValue.size(); ++i) {
            const JsonValue& item = value.arrayValue[i];
            const JsonValue* start = objectFieldAny(item, "start", "Start");
            const JsonValue* sz = objectFieldAny(item, "size", "Size");
            const JsonValue* flags = objectFieldAny(item, "flags", "Flags");
            const JsonValue* name = objectFieldAny(item, "name", "Name");
            if (start == nullptr || sz == nullptr) {
                continue;
            }
            DtRegisterEntry entry =
                makeEntry(nodePath, key, compatible, RegisterKind::PmapIoRange, i);
            entry.physAddr = jsonNumberAsU64(*start);
            entry.size = jsonNumberAsU64(*sz);
            entry.label = jsonStringOrEmpty(name);
            if (flags != nullptr) {
                entry.flags = static_cast<uint32_t>(jsonNumberAsU64(*flags));
            }
            pushRegister(entry, inventory);
        }
        return;
    }
    if (key == "ps-regs" && value.type == JsonType::Array) {
        for (size_t i = 0; i < value.arrayValue.size(); ++i) {
            const JsonValue& item = value.arrayValue[i];
            const JsonValue* reg = objectFieldAny(item, "reg", "Reg");
            const JsonValue* off = objectFieldAny(item, "off", "Off");
            const JsonValue* unk = objectFieldAny(item, "unk", "Unk");
            if (reg == nullptr || off == nullptr) {
                continue;
            }
            DtRegisterEntry entry =
                makeEntry(nodePath, key, compatible, RegisterKind::PmgrPsReg, i);
            entry.regBank = static_cast<uint32_t>(jsonNumberAsU64(*reg));
            entry.offset = jsonNumberAsU64(*off);
            if (unk != nullptr) {
                entry.unk = static_cast<uint32_t>(jsonNumberAsU64(*unk));
            }
            std::ostringstream detail;
            detail << "bank=0x" << std::hex << entry.regBank << " off=0x" << entry.offset;
            if (unk != nullptr) {
                detail << " unk=0x" << entry.unk;
            }
            entry.detail = detail.str();
            pushRegister(entry, inventory);
        }
        return;
    }
    if (key == "devices" && value.type == JsonType::Array) {
        for (size_t i = 0; i < value.arrayValue.size(); ++i) {
            const JsonValue& item = value.arrayValue[i];
            DtRegisterEntry entry =
                makeEntry(nodePath, key, compatible, RegisterKind::PmgrDevice, i);
            entry.label = jsonStringOrEmpty(objectFieldAny(item, "name", "Name"));
            const JsonValue* map = objectFieldAny(item, "map", "Map");
            const JsonValue* index = objectFieldAny(item, "index", "Index");
            const JsonValue* flag = objectFieldAny(item, "flag", "Flag");
            if (map != nullptr) {
                entry.regBank = static_cast<uint32_t>(jsonNumberAsU64(*map));
            }
            if (index != nullptr) {
                entry.index = static_cast<size_t>(jsonNumberAsU64(*index));
            }
            if (flag != nullptr) {
                entry.flags = static_cast<uint32_t>(jsonNumberAsU64(*flag));
            }
            const JsonValue* id1 = objectFieldAny(item, "id1", "ID1");
            const JsonValue* id2 = objectFieldAny(item, "id2", "ID2");
            const JsonValue* alias = objectFieldAny(item, "alias", "Alias");
            std::ostringstream detail;
            detail << "index=" << entry.index << " map=" << entry.regBank
                   << " flag=" << entry.flags;
            if (id1 != nullptr) {
                detail << " id1=" << jsonNumberAsU64(*id1);
            }
            if (id2 != nullptr) {
                detail << " id2=" << jsonNumberAsU64(*id2);
            }
            if (alias != nullptr) {
                detail << " alias=" << jsonNumberAsU64(*alias);
            }
            entry.detail = detail.str();
            pushRegister(entry, inventory);
        }
        return;
    }
    if (key == "regions" && value.type == JsonType::Array) {
        for (size_t i = 0; i < value.arrayValue.size(); ++i) {
            const JsonValue& item = value.arrayValue[i];
            const JsonValue* start = objectFieldAny(item, "start", "Start");
            const JsonValue* end = objectFieldAny(item, "end", "End");
            if (start == nullptr || end == nullptr) {
                continue;
            }
            DtRegisterEntry entry =
                makeEntry(nodePath, key, compatible, RegisterKind::RegionSpan, i);
            entry.physAddr = jsonNumberAsU64(*start);
            entry.end = jsonNumberAsU64(*end);
            if (entry.end >= entry.physAddr) {
                entry.size = entry.end - entry.physAddr;
            }
            pushRegister(entry, inventory);
        }
        return;
    }
    if (key == "AAPL,phandle" && value.type == JsonType::Number) {
        DtRegisterEntry entry = makeEntry(nodePath, key, compatible, RegisterKind::Phandle, 0);
        entry.phandle = static_cast<uint32_t>(jsonNumberAsU64(value));
        entry.detail = "phandle";
        pushRegister(entry, inventory);
        return;
    }
    if (key == "interrupts" || key == "interrupt-parent") {
        if (value.type == JsonType::Array) {
            for (size_t i = 0; i < value.arrayValue.size(); ++i) {
                if (value.arrayValue[i].type != JsonType::Number) {
                    continue;
                }
                DtRegisterEntry entry =
                    makeEntry(nodePath, key, compatible, RegisterKind::Interrupt, i);
                entry.physAddr = jsonNumberAsU64(value.arrayValue[i]);
                pushRegister(entry, inventory);
            }
        } else if (value.type == JsonType::Number) {
            DtRegisterEntry entry = makeEntry(nodePath, key, compatible, RegisterKind::Interrupt, 0);
            entry.physAddr = jsonNumberAsU64(value);
            pushRegister(entry, inventory);
        }
        return;
    }

    if (value.type == JsonType::Number && (key.find("reg") != std::string::npos ||
                                           key.find("addr") != std::string::npos ||
                                           key.find("mmio") != std::string::npos)) {
        DtRegisterEntry entry =
            makeEntry(nodePath, key, compatible, RegisterKind::NumericProperty, 0);
        entry.physAddr = jsonNumberAsU64(value);
        pushRegister(entry, inventory);
    }
}

bool isRegisterPropertyName(const std::string& key) {
    return key == "reg" || key == "reg-private" || key == "pmap-io-ranges" || key == "ps-regs" ||
           key == "devices" || key == "regions" || key == "AAPL,phandle" || key == "interrupts" ||
           key == "interrupt-parent" || key.find("reg") != std::string::npos ||
           key.find("mmio") != std::string::npos || key.find("addr") != std::string::npos;
}

void walkNode(const JsonValue& props, const std::string& nodePath, RegisterInventory* inventory) {
    if (props.type != JsonType::Object) {
        return;
    }

    if (nodePath.find("device-tree") != std::string::npos) {
        extractSummaryFields(props, &inventory->summary);
    }

    const std::string compatible = compatibleFromProps(props);
    DtNodeRecord node;
    node.path = nodePath;
    node.compatible = compatible;
    const JsonValue* nameField = objectField(props, "name");
    if (nameField != nullptr) {
        node.name = jsonStringOrEmpty(nameField);
    } else {
        const size_t slash = nodePath.find_last_of('/');
        node.name = slash == std::string::npos ? nodePath : nodePath.substr(slash + 1);
    }

    for (std::map<std::string, JsonValue>::const_iterator it = props.objectValue.begin();
         it != props.objectValue.end(); ++it) {
        if (it->first == "children") {
            continue;
        }
        node.propertyNames.push_back(it->first);
        if (isRegisterPropertyName(it->first)) {
            extractKnownProperty(it->first, it->second, nodePath, compatible, inventory);
        }
    }
    inventory->nodes.push_back(node);

    const JsonValue* children = objectField(props, "children");
    if (children != nullptr && children->type == JsonType::Array) {
        for (size_t i = 0; i < children->arrayValue.size(); ++i) {
            const JsonValue& childWrapper = children->arrayValue[i];
            if (childWrapper.type != JsonType::Object) {
                continue;
            }
            for (std::map<std::string, JsonValue>::const_iterator it = childWrapper.objectValue.begin();
                 it != childWrapper.objectValue.end(); ++it) {
                walkNode(it->second, joinPath(nodePath, it->first), inventory);
            }
        }
    }

    for (std::map<std::string, JsonValue>::const_iterator it = props.objectValue.begin();
         it != props.objectValue.end(); ++it) {
        if (it->first == "children") {
            continue;
        }
        if (it->second.type == JsonType::Object) {
            walkNode(it->second, joinPath(nodePath, it->first), inventory);
        }
    }
}

void walkRoot(const JsonValue& root, RegisterInventory* inventory) {
    if (root.type != JsonType::Object) {
        return;
    }
    for (std::map<std::string, JsonValue>::const_iterator it = root.objectValue.begin();
         it != root.objectValue.end(); ++it) {
        if (it->second.type == JsonType::Object) {
            walkNode(it->second, it->first, inventory);
        }
    }
}

} /* anonymous */

RegisterInventory buildRegisterInventory(const JsonValue& root) {
    RegisterInventory inventory;
    walkRoot(root, &inventory);
    return inventory;
}

std::vector<MmioRegion> RegisterInventory::mmioRegions(bool interestingOnly) const {
    std::vector<MmioRegion> out;
    for (size_t i = 0; i < registers.size(); ++i) {
        const DtRegisterEntry& reg = registers[i];
        if (reg.kind == RegisterKind::Interrupt || reg.kind == RegisterKind::Phandle ||
            reg.kind == RegisterKind::PmgrDevice) {
            continue;
        }
        if (reg.physAddr == 0 && reg.size == 0 && reg.kind != RegisterKind::PmgrPsReg) {
            continue;
        }
        MmioRegion region;
        region.nodePath = reg.nodePath;
        region.compatible = reg.compatible;
        region.label = reg.label.empty() ? reg.property : reg.label;
        region.physAddr = reg.physAddr;
        region.size = reg.size;
        region.tag = reg.tag;
        if (interestingOnly && region.tag == MmioTag::Generic) {
            continue;
        }
        out.push_back(region);
    }
    return out;
}

size_t RegisterInventory::countByKind(RegisterKind kind) const {
    size_t count = 0;
    for (size_t i = 0; i < registers.size(); ++i) {
        if (registers[i].kind == kind) {
            ++count;
        }
    }
    return count;
}

void logRegisterInventorySummary(const RegisterInventory& inventory) {
    Logger::info("  [DeviceTree] nodes=" + std::to_string(inventory.nodes.size()) + " registers=" +
                 std::to_string(inventory.registers.size()));
    Logger::info("  [DeviceTree]   reg=" + std::to_string(inventory.countByKind(RegisterKind::Reg)) +
                 " pmap-io=" + std::to_string(inventory.countByKind(RegisterKind::PmapIoRange)) +
                 " ps-regs=" + std::to_string(inventory.countByKind(RegisterKind::PmgrPsReg)) +
                 " pmgr-dev=" + std::to_string(inventory.countByKind(RegisterKind::PmgrDevice)) +
                 " regions=" + std::to_string(inventory.countByKind(RegisterKind::RegionSpan)));
}

void logRegisterInventoryVerbose(const RegisterInventory& inventory, size_t maxEntries) {
    const size_t limit = maxEntries == 0 ? inventory.registers.size()
                                         : std::min(maxEntries, inventory.registers.size());
    for (size_t i = 0; i < limit; ++i) {
        const DtRegisterEntry& reg = inventory.registers[i];
        std::ostringstream oss;
        oss << "  [DeviceTree] #" << i << " " << reg.nodePath << " " << reg.property << " ["
            << registerKindToString(reg.kind) << "]";
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
        if (!reg.label.empty()) {
            oss << " label=" << reg.label;
        }
        if (!reg.detail.empty()) {
            oss << " " << reg.detail;
        }
        Logger::info(oss.str());
    }
    if (limit < inventory.registers.size()) {
        Logger::info("  [DeviceTree] ... " + std::to_string(inventory.registers.size() - limit) +
                     " more register entries");
    }
}

void applyInventoryToCatalog(const RegisterInventory& inventory, bool interestingOnly,
                             DeviceTreeCatalog* catalog) {
    if (catalog == nullptr) {
        return;
    }
    catalog->summary = inventory.summary;
    catalog->nodes = inventory.nodes;
    catalog->registers = inventory.registers;
    catalog->regions = inventory.mmioRegions(interestingOnly);
    catalog->success = true;
}

} /* namespace devicetree */
} /* namespace PP */
