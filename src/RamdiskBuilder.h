/*
 * RamdiskBuilder.h
 *
 * High-level custom HFS+ ramdisk assembly from overlay directories.
 */

#ifndef RAMDISK_BUILDER_H_
#define RAMDISK_BUILDER_H_

#include "RamdiskTypes.h"
#include "HfsPlusWriter.h"

#include <string>
#include <vector>

namespace PP {

class RamdiskBuilder {
public:
    explicit RamdiskBuilder(const RamdiskOptions& options);

    bool addOverlayDirectory(const std::string& overlayRoot);
    bool addFile(const std::string& absolutePath, const std::vector<uint8_t>& data,
                 uint16_t fileMode = 0100644);
    bool build(std::vector<uint8_t>* image);
    bool buildToFile(const std::string& path);

private:
    RamdiskOptions mOptions;
    HfsPlusWriter mWriter;

    static std::string overlayPathToHfs(const std::string& overlayRoot, const std::string& filePath);
};

} /* namespace PP */

#endif /* RAMDISK_BUILDER_H_ */
