/*
 * HfsPlusWriter.h
 *
 * Minimal in-memory HFS+ volume creator (flat catalog, no mount).
 */

#ifndef HFS_PLUS_WRITER_H_
#define HFS_PLUS_WRITER_H_

#include "RamdiskTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PP {

class HfsPlusWriter {
public:
    explicit HfsPlusWriter(const RamdiskOptions& options);

    bool addFile(const std::string& absolutePath, const std::vector<uint8_t>& data,
                 uint16_t fileMode = 0100755);
    bool build(std::vector<uint8_t>* image);
    bool buildToFile(const std::string& path);

private:
    struct CatalogItem {
        enum class Kind { Folder, File };
        Kind kind = Kind::File;
        std::string name;
        uint32_t parentCnid = 2;
        uint32_t cnid = 0;
        std::vector<uint8_t> data;
        uint16_t fileMode = 0100755;
        uint32_t dataStartBlock = 0;
        uint32_t dataBlockCount = 0;
    };

    RamdiskOptions mOptions;
    std::vector<CatalogItem> mItems;
    uint32_t mNextCnid = 16;

    bool ensureParentFolders(const std::string& absolutePath, uint32_t* parentCnid);
    uint32_t totalBlocks() const;
    static uint32_t hfsNow();
};

} /* namespace PP */

#endif /* HFS_PLUS_WRITER_H_ */
