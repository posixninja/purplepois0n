/*
 * RamdiskPackager.h
 */

#ifndef RAMDISK_PACKAGER_H_
#define RAMDISK_PACKAGER_H_

#include "RamdiskTypes.h"

#include <string>

namespace PP {

struct RamdiskPackagerResult {
    std::string dmgPath;
    std::string im4pPath;
    std::string img4Path;
};

/** Build blank or overlay HFS+ to dmgPath. */
bool buildRamdiskDmg(const RamdiskOptions& options, const std::string& overlayDir,
                     const std::vector<RamdiskStageEntry>& stagedFiles,
                     const std::string& dmgPath);

/**
 * Extract stock RestoreRamDisk from IPSW, apply overlay, repack IM4P (and optional IMG4).
 * @p ident Erase or Update
 */
bool packRamdiskFromIpsw(const RamdiskOptions& options, const std::string& ipswPath,
                         const std::string& ident, const std::string& overlayDir,
                         const std::vector<RamdiskStageEntry>& stagedFiles,
                         const std::string& workDir, RamdiskPackagerResult* result);

/** Wrap raw .dmg as lzss IM4P rdsk. */
bool wrapDmgAsIm4p(const std::string& dmgPath, const std::string& im4pPath);

/** Extract IPSW component matching @p pattern (e.g. iBSS.*im4p) into @p workDir. */
bool locateIpswIm4pComponent(const std::string& ipswPath, const std::string& workDir,
                             const std::string& pattern, std::string* outPath);

} /* namespace PP */

#endif /* RAMDISK_PACKAGER_H_ */
