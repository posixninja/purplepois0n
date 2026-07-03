/*
 * MmioTypes.h
 *
 * MMIO region descriptors extracted from Apple DeviceTree.
 */

#ifndef DEVICETREE_MMIO_TYPES_H_
#define DEVICETREE_MMIO_TYPES_H_

#include <cstdint>
#include <string>
#include <vector>

namespace PP {
namespace devicetree {

enum class MmioTag {
    Generic,
    AgxGpu,
    Ane,
    ArmIo,
    Pmgr,
    Display,
    OtherCoproc
};

struct MmioRegion {
    std::string nodePath;
    std::string compatible;
    std::string label;
    uint64_t physAddr = 0;
    uint64_t size = 0;
    MmioTag tag = MmioTag::Generic;
};

struct DeviceTreeSummary {
    std::string model;
    std::string boardConfig;
    std::string productName;
    std::string socName;
    std::string socGeneration;
    std::string deviceType;
    /** /product/has-virtualization from DeviceTree (A15+ hypervisor gate). */
    bool hasVirtualization = false;
    /** Heuristic: SPTM-era page authority replaces classic PPL. */
    bool pageMonitorPresent = false;
};

enum class RegisterKind {
    Reg,
    RegPrivate,
    PmapIoRange,
    PmgrWindow,
    PmgrPsReg,
    PmgrDevice,
    RegionSpan,
    Interrupt,
    Phandle,
    ScalarMmio,
    NumericProperty
};

struct DtNodeRecord {
    std::string path;
    std::string name;
    std::string compatible;
    std::vector<std::string> propertyNames;
};

struct DtRegisterEntry {
    std::string nodePath;
    std::string property;
    std::string compatible;
    RegisterKind kind = RegisterKind::NumericProperty;
    size_t index = 0;

    uint64_t physAddr = 0;
    uint64_t size = 0;
    uint64_t offset = 0;
    uint64_t end = 0;
    uint32_t flags = 0;
    uint32_t regBank = 0;
    uint32_t unk = 0;
    uint32_t phandle = 0;

    std::string label;
    std::string detail;

    MmioTag tag = MmioTag::Generic;
};

struct DeviceTreeCatalog {
    bool success = false;
    std::string sourcePath;
    std::string error;
    DeviceTreeSummary summary;
    std::vector<MmioRegion> regions;
    std::vector<DtNodeRecord> nodes;
    std::vector<DtRegisterEntry> registers;
    std::vector<MmioRegion> interestingRegions() const;
};

const char* mmioTagToString(MmioTag tag);
const char* registerKindToString(RegisterKind kind);
MmioTag classifyMmioHint(const std::string& text);

} /* namespace devicetree */
} /* namespace PP */

#endif /* DEVICETREE_MMIO_TYPES_H_ */
