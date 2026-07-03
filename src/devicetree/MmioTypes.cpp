/*
 * MmioTypes.cpp
 */

#include "devicetree/MmioTypes.h"

#include <cctype>

namespace PP {
namespace devicetree {

namespace {

std::string lowerCopy(std::string text) {
    for (size_t i = 0; i < text.size(); ++i) {
        text[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    }
    return text;
}

bool hintContains(const std::string& haystack, const char* needle) {
    return haystack.find(needle) != std::string::npos;
}

} /* anonymous */

const char* mmioTagToString(MmioTag tag) {
    switch (tag) {
        case MmioTag::AgxGpu:
            return "agx-gpu";
        case MmioTag::Ane:
            return "ane";
        case MmioTag::ArmIo:
            return "arm-io";
        case MmioTag::Pmgr:
            return "pmgr";
        case MmioTag::Display:
            return "display";
        case MmioTag::OtherCoproc:
            return "coproc";
        case MmioTag::Generic:
        default:
            return "generic";
    }
}

MmioTag classifyMmioHint(const std::string& text) {
    const std::string lower = lowerCopy(text);
    if (hintContains(lower, "agx") || hintContains(lower, "gpu") || hintContains(lower, "gfx")) {
        return MmioTag::AgxGpu;
    }
    if (hintContains(lower, "ane")) {
        return MmioTag::Ane;
    }
    if (hintContains(lower, "arm-io")) {
        return MmioTag::ArmIo;
    }
    if (hintContains(lower, "pmgr")) {
        return MmioTag::Pmgr;
    }
    if (hintContains(lower, "display") || hintContains(lower, "dcp")) {
        return MmioTag::Display;
    }
    if (hintContains(lower, "apple,") || hintContains(lower, "coproc")) {
        return MmioTag::OtherCoproc;
    }
    return MmioTag::Generic;
}

const char* registerKindToString(RegisterKind kind) {
    switch (kind) {
        case RegisterKind::Reg:
            return "reg";
        case RegisterKind::RegPrivate:
            return "reg-private";
        case RegisterKind::PmapIoRange:
            return "pmap-io-ranges";
        case RegisterKind::PmgrWindow:
            return "pmgr-window";
        case RegisterKind::PmgrPsReg:
            return "ps-regs";
        case RegisterKind::PmgrDevice:
            return "pmgr-device";
        case RegisterKind::RegionSpan:
            return "regions";
        case RegisterKind::Interrupt:
            return "interrupt";
        case RegisterKind::Phandle:
            return "phandle";
        case RegisterKind::ScalarMmio:
            return "scalar-mmio";
        case RegisterKind::NumericProperty:
        default:
            return "numeric";
    }
}

std::vector<MmioRegion> DeviceTreeCatalog::interestingRegions() const {
    std::vector<MmioRegion> out;
    for (size_t i = 0; i < regions.size(); ++i) {
        if (regions[i].tag != MmioTag::Generic) {
            out.push_back(regions[i]);
        }
    }
    return out;
}

} /* namespace devicetree */
} /* namespace PP */
