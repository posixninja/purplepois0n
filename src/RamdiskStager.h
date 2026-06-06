/*
 * RamdiskStager.h
 *
 * Copy host files (including arm64 Mach-O from the Mac) into a RamdiskBuilder.
 */

#ifndef RAMDISK_STAGER_H_
#define RAMDISK_STAGER_H_

#include "RamdiskTypes.h"

namespace PP {

class RamdiskBuilder;

/** Read @p entry from disk and add to @p builder (optional arm64 Mach-O check). */
bool stageHostFile(RamdiskBuilder* builder, const RamdiskStageEntry& entry);

bool stageHostFiles(RamdiskBuilder* builder, const std::vector<RamdiskStageEntry>& entries);

} /* namespace PP */

#endif /* RAMDISK_STAGER_H_ */
