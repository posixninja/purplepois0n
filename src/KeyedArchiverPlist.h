/*
 * KeyedArchiverPlist.h
 *
 * Minimal NSKeyedArchiver metadata extraction for iOS backup Manifest.db
 * `file` BLOBs. Resolves NSDictionary ($objects) entries only — not a full
 * unarchiver.
 */

#ifndef KEYED_ARCHIVER_PLIST_H_
#define KEYED_ARCHIVER_PLIST_H_

#include <cstdint>
#include <plist/plist.h>
#include <string>
#include <vector>

namespace PP {

/** File metadata fields extracted from a Manifest.db `file` BLOB. */
struct KeyedArchiverFileMetadata {
    bool valid = false;
    uint64_t size = 0;
    uint64_t mtime = 0;
    uint64_t ctime = 0;
    bool hasEncryptionKey = false;
    std::string encryptionKey;
};

/** Parse plain or NSKeyedArchiver plist bytes into file metadata. */
class KeyedArchiverPlist {
public:
    static KeyedArchiverFileMetadata parseFileMetadata(const std::vector<uint8_t>& blob);

private:
    static KeyedArchiverFileMetadata parsePlainDict(plist_t root);
    static KeyedArchiverFileMetadata parseKeyedArchiver(plist_t root);
};

} /* namespace PP */

#endif /* KEYED_ARCHIVER_PLIST_H_ */
