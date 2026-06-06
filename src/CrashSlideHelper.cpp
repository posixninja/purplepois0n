/*
 * CrashSlideHelper.cpp
 */

#include "CrashSlideHelper.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

namespace PP {

namespace {

bool parseHexU64(const std::string& text, uint64_t* out) {
    if (text.empty() || out == nullptr) {
        return false;
    }
    char* end = nullptr;
    const uint64_t value = strtoull(text.c_str(), &end, 16);
    if (end == text.c_str()) {
        return false;
    }
    *out = value;
    return true;
}

bool extractSlideFromLine(const std::string& line, uint64_t* slideOut) {
    static const char* markers[] = {
        "(slide=0x", "(slide=0X", "(slide 0x", "(slide 0X", nullptr};
    for (int i = 0; markers[i] != nullptr; ++i) {
        const size_t pos = line.find(markers[i]);
        if (pos == std::string::npos) {
            continue;
        }
        size_t start = pos + strlen(markers[i]);
        while (start < line.size() && line[start] == ' ') {
            ++start;
        }
        size_t end = start;
        while (end < line.size() && std::isxdigit(static_cast<unsigned char>(line[end]))) {
            ++end;
        }
        if (end <= start) {
            return false;
        }
        return parseHexU64(line.substr(start, end - start), slideOut);
    }
    return false;
}

int parseFrameIndex(const std::string& line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos || colon > 8) {
        return -1;
    }
    size_t start = 0;
    while (start < colon && std::isspace(static_cast<unsigned char>(line[start]))) {
        ++start;
    }
    std::string indexText = line.substr(start, colon - start);
    if (indexText.empty()) {
        return -1;
    }
    for (size_t i = 0; i < indexText.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(indexText[i]))) {
            return -1;
        }
    }
    return std::atoi(indexText.c_str());
}

std::string parseImageName(const std::string& line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) {
        return std::string();
    }
    size_t start = colon + 1;
    while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
        ++start;
    }
    size_t end = line.find("(slide", start);
    if (end == std::string::npos) {
        end = line.size();
    }
    while (end > start && std::isspace(static_cast<unsigned char>(line[end - 1]))) {
        --end;
    }
    std::string name = line.substr(start, end - start);
    const size_t addrPos = name.find(" 0x");
    if (addrPos != std::string::npos) {
        name = name.substr(0, addrPos);
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) {
            name.pop_back();
        }
    }
    return name;
}

bool parseLoadAddress(const std::string& line, uint64_t* addressOut) {
    size_t slidePos = line.find("(slide");
    if (slidePos == std::string::npos) {
        return false;
    }
    const std::string before = line.substr(0, slidePos);
    const size_t hexPos = before.rfind("0x");
    if (hexPos == std::string::npos) {
        return false;
    }
    size_t start = hexPos + 2;
    size_t end = start;
    while (end < before.size() && std::isxdigit(static_cast<unsigned char>(before[end]))) {
        ++end;
    }
    if (end <= start) {
        return false;
    }
    return parseHexU64(before.substr(start, end - start), addressOut);
}

} /* anonymous namespace */

bool parseCrashSlideFile(const std::string& path, CrashSlideSummary* summary) {
    if (summary == nullptr) {
        return false;
    }
    summary->entries.clear();
    summary->sourcePath = path;

    std::ifstream in(path.c_str());
    if (!in.good()) {
        return false;
    }

    std::map<std::string, CrashSlideEntry> deduped;
    std::string line;
    while (std::getline(in, line)) {
        uint64_t slide = 0;
        if (!extractSlideFromLine(line, &slide)) {
            continue;
        }
        CrashSlideEntry entry;
        entry.frameIndex = parseFrameIndex(line);
        entry.imageName = parseImageName(line);
        entry.slide = slide;
        uint64_t loadAddress = 0;
        if (parseLoadAddress(line, &loadAddress)) {
            entry.loadAddress = loadAddress;
            entry.hasLoadAddress = true;
        }
        if (entry.imageName.empty()) {
            entry.imageName = "unknown";
        }
        if (deduped.find(entry.imageName) == deduped.end()) {
            deduped[entry.imageName] = entry;
        }
    }

    for (std::map<std::string, CrashSlideEntry>::const_iterator it = deduped.begin();
         it != deduped.end(); ++it) {
        summary->entries.push_back(it->second);
    }
    return !summary->entries.empty();
}

void printCrashSlideSummary(const CrashSlideSummary& summary, std::ostream& out) {
    out << "\n=== Crash slide analysis (research only) ===" << std::endl;
    out << "Source: " << summary.sourcePath << std::endl;
    out << "Images with slide annotations: " << summary.entries.size() << std::endl;
    out << std::endl;

    for (size_t i = 0; i < summary.entries.size(); ++i) {
        const CrashSlideEntry& entry = summary.entries[i];
        out << "  " << entry.imageName << std::endl;
        out << "    slide: 0x" << std::hex << entry.slide << std::dec;
        if (entry.hasLoadAddress) {
            out << "  load: 0x" << std::hex << entry.loadAddress << std::dec;
        }
        if (entry.frameIndex >= 0) {
            out << "  (frame " << entry.frameIndex << ")";
        }
        out << std::endl;
    }

    out << "\nAbsinthe-era note: correlate slides with dyld shared cache / Mach-O offline."
        << std::endl;
    out << "purplepois0n does not use crash logs for live jailbreak staging — see docs/SUPPORT.md."
        << std::endl;
}

} /* namespace PP */
