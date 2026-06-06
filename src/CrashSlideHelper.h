/*
 * CrashSlideHelper.h
 *
 * Offline crash-log / symbolicate slide extraction (absinthe-era research).
 */

#ifndef CRASH_SLIDE_HELPER_H_
#define CRASH_SLIDE_HELPER_H_

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace PP {

struct CrashSlideEntry {
    int frameIndex = -1;
    std::string imageName;
    uint64_t slide = 0;
    uint64_t loadAddress = 0;
    bool hasLoadAddress = false;
};

struct CrashSlideSummary {
    std::vector<CrashSlideEntry> entries;
    std::string sourcePath;
};

/** Parse ipsw-style symbolicate lines containing (slide=0x...) or (slide 0x...). */
bool parseCrashSlideFile(const std::string& path, CrashSlideSummary* summary);

void printCrashSlideSummary(const CrashSlideSummary& summary, std::ostream& out);

} /* namespace PP */

#endif /* CRASH_SLIDE_HELPER_H_ */
