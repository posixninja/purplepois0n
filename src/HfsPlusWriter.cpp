/*
 * HfsPlusWriter.cpp
 */

#include "HfsPlusWriter.h"

#include "Logger.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>

namespace PP {

namespace {

constexpr uint32_t kCatalogBlock = 3;
constexpr uint32_t kCatalogBlockCount = 2;
constexpr uint32_t kAllocBlock = 1;
constexpr uint32_t kExtentsBlock = 2;
constexpr uint32_t kFirstDataBlock = 5;

constexpr uint32_t kRootParentCnid = 1;
constexpr uint32_t kRootFolderCnid = 2;

void writeBE16(std::vector<uint8_t>* out, uint16_t value) {
    out->push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    out->push_back(static_cast<uint8_t>(value & 0xff));
}

void writeBE32(std::vector<uint8_t>* out, uint32_t value) {
    out->push_back(static_cast<uint8_t>((value >> 24) & 0xff));
    out->push_back(static_cast<uint8_t>((value >> 16) & 0xff));
    out->push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    out->push_back(static_cast<uint8_t>(value & 0xff));
}

void writeBE64(std::vector<uint8_t>* out, uint64_t value) {
    writeBE32(out, static_cast<uint32_t>(value >> 32));
    writeBE32(out, static_cast<uint32_t>(value & 0xffffffffULL));
}

void patchBE16(std::vector<uint8_t>* out, size_t offset, uint16_t value) {
    if (offset + 1 >= out->size()) {
        return;
    }
    (*out)[offset] = static_cast<uint8_t>((value >> 8) & 0xff);
    (*out)[offset + 1] = static_cast<uint8_t>(value & 0xff);
}

void patchBE32(std::vector<uint8_t>* out, size_t offset, uint32_t value) {
    if (offset + 3 >= out->size()) {
        return;
    }
    (*out)[offset] = static_cast<uint8_t>((value >> 24) & 0xff);
    (*out)[offset + 1] = static_cast<uint8_t>((value >> 16) & 0xff);
    (*out)[offset + 2] = static_cast<uint8_t>((value >> 8) & 0xff);
    (*out)[offset + 3] = static_cast<uint8_t>(value & 0xff);
}

std::vector<uint16_t> utf8ToUtf16BE(const std::string& text) {
    std::vector<uint16_t> out;
    for (size_t i = 0; i < text.size(); ++i) {
        out.push_back(static_cast<uint16_t>(static_cast<unsigned char>(text[i])));
    }
    return out;
}

void appendCatalogKey(std::vector<uint8_t>* out, uint32_t parentCnid, const std::string& name) {
    const std::vector<uint16_t> uni = utf8ToUtf16BE(name);
    /* go-apfs/ipsw: keyLength is parentID+nodeName payload only (excludes length word). */
    const uint16_t keyLen = static_cast<uint16_t>(4 + 2 + uni.size() * 2);
    writeBE16(out, keyLen);
    writeBE32(out, parentCnid);
    writeBE16(out, static_cast<uint16_t>(uni.size()));
    for (size_t i = 0; i < uni.size(); ++i) {
        writeBE16(out, uni[i]);
    }
}

void appendThreadKey(std::vector<uint8_t>* out, uint32_t cnid) {
    writeBE16(out, 6);
    writeBE32(out, cnid);
    writeBE16(out, 0);
}

void appendFork(std::vector<uint8_t>* out, uint64_t logicalSize, uint32_t startBlock,
                uint32_t blockCount) {
    writeBE64(out, logicalSize);
    writeBE32(out, 0);
    writeBE32(out, blockCount);
    writeBE32(out, startBlock);
    writeBE32(out, blockCount);
    for (int i = 1; i < 8; ++i) {
        writeBE32(out, 0);
        writeBE32(out, 0);
    }
}

std::string normalizePath(const std::string& path) {
    if (path.empty() || path[0] != '/') {
        return std::string();
    }
    std::string out;
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '/' && !out.empty() && out[out.size() - 1] == '/') {
            continue;
        }
        out += path[i];
    }
    if (out.size() > 1 && out[out.size() - 1] == '/') {
        out.erase(out.size() - 1);
    }
    return out;
}

struct RawCatalogRecord {
    std::vector<uint8_t> key;
    std::vector<uint8_t> data;
};

bool keyLess(const RawCatalogRecord& a, const RawCatalogRecord& b) {
    return a.key < b.key;
}

void appendThreadName(std::vector<uint8_t>* out, const std::string& name) {
    const std::vector<uint16_t> uni = utf8ToUtf16BE(name);
    writeBE16(out, static_cast<uint16_t>(uni.size()));
    for (size_t i = 0; i < uni.size(); ++i) {
        writeBE16(out, uni[i]);
    }
}

void appendFolderThreadData(std::vector<uint8_t>* out, uint32_t parentCnid,
                            const std::string& name) {
    writeBE16(out, 3);
    writeBE16(out, 0);
    writeBE32(out, parentCnid);
    appendThreadName(out, name);
}

void appendFileThreadData(std::vector<uint8_t>* out, uint32_t parentCnid,
                          const std::string& name) {
    writeBE16(out, 4);
    writeBE16(out, 0);
    writeBE32(out, parentCnid);
    appendThreadName(out, name);
}

} /* anonymous */

HfsPlusWriter::HfsPlusWriter(const RamdiskOptions& options) : mOptions(options) {
    if (mOptions.blockSize == 0) {
        mOptions.blockSize = 4096;
    }
    if (mOptions.sizeBytes < mOptions.blockSize * 8) {
        mOptions.sizeBytes = 16ULL * 1024ULL * 1024ULL;
    }
}

uint32_t HfsPlusWriter::hfsNow() {
    const uint64_t unixNow = static_cast<uint64_t>(std::time(nullptr));
    return static_cast<uint32_t>(unixNow + 2082844800ULL);
}

uint32_t HfsPlusWriter::totalBlocks() const {
    return static_cast<uint32_t>(mOptions.sizeBytes / mOptions.blockSize);
}

bool HfsPlusWriter::ensureParentFolders(const std::string& absolutePath, uint32_t* parentCnid) {
    const std::string path = normalizePath(absolutePath);
    if (path.empty() || path == "/") {
        return false;
    }
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos || slash == 0) {
        *parentCnid = kRootFolderCnid;
        return true;
    }
    const std::string dirPath = path.substr(0, slash);
    const std::string leafName = path.substr(slash + 1);
    if (leafName.empty()) {
        return false;
    }

    uint32_t currentParent = kRootFolderCnid;
    size_t pos = 1;
    while (pos < dirPath.size()) {
        const size_t next = dirPath.find('/', pos);
        const std::string component =
            dirPath.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
        if (component.empty()) {
            break;
        }
        bool found = false;
        for (size_t i = 0; i < mItems.size(); ++i) {
            if (mItems[i].kind == CatalogItem::Kind::Folder && mItems[i].name == component &&
                mItems[i].parentCnid == currentParent) {
                currentParent = mItems[i].cnid;
                found = true;
                break;
            }
        }
        if (!found) {
            CatalogItem folder;
            folder.kind = CatalogItem::Kind::Folder;
            folder.name = component;
            folder.parentCnid = currentParent;
            folder.cnid = mNextCnid++;
            mItems.push_back(folder);
            currentParent = folder.cnid;
        }
        if (next == std::string::npos) {
            break;
        }
        pos = next + 1;
    }
    *parentCnid = currentParent;
    return true;
}

bool HfsPlusWriter::addFile(const std::string& absolutePath, const std::vector<uint8_t>& data,
                            uint16_t fileMode) {
    const std::string path = normalizePath(absolutePath);
    if (path.empty() || path == "/") {
        return false;
    }
    uint32_t parentCnid = kRootFolderCnid;
    if (!ensureParentFolders(path, &parentCnid)) {
        return false;
    }
    const size_t slash = path.find_last_of('/');
    const std::string name = path.substr(slash + 1);
    if (name.empty()) {
        return false;
    }

    CatalogItem item;
    item.kind = CatalogItem::Kind::File;
    item.name = name;
    item.parentCnid = parentCnid;
    item.cnid = mNextCnid++;
    item.data = data;
    item.fileMode = fileMode;
    mItems.push_back(item);
    return true;
}

bool HfsPlusWriter::build(std::vector<uint8_t>* image) {
    if (image == nullptr) {
        return false;
    }
    const uint32_t blockSize = mOptions.blockSize;
    const uint32_t total = totalBlocks();
    if (total < kFirstDataBlock + 1) {
        Logger::error("  [Ramdisk] volume too small");
        return false;
    }

    uint32_t nextDataBlock = kFirstDataBlock;
    for (size_t i = 0; i < mItems.size(); ++i) {
        if (mItems[i].kind != CatalogItem::Kind::File) {
            continue;
        }
        const uint64_t size = mItems[i].data.size();
        const uint32_t blocks =
            static_cast<uint32_t>((size + blockSize - 1) / blockSize);
        if (blocks == 0) {
            mItems[i].dataBlockCount = 0;
            mItems[i].dataStartBlock = 0;
            continue;
        }
        if (nextDataBlock + blocks > total) {
            Logger::error("  [Ramdisk] volume full");
            return false;
        }
        mItems[i].dataStartBlock = nextDataBlock;
        mItems[i].dataBlockCount = blocks;
        nextDataBlock += blocks;
    }

    image->assign(static_cast<size_t>(total) * blockSize, 0);

    const uint32_t now = hfsNow();
    std::vector<RawCatalogRecord> records;

    {
        RawCatalogRecord threadRec;
        appendThreadKey(&threadRec.key, kRootFolderCnid);
        appendFolderThreadData(&threadRec.data, kRootParentCnid, "");
        records.push_back(threadRec);

        RawCatalogRecord folderRec;
        appendCatalogKey(&folderRec.key, kRootParentCnid, "");
        writeBE16(&folderRec.data, 1);
        writeBE16(&folderRec.data, 0);
        writeBE32(&folderRec.data, 0);
        writeBE32(&folderRec.data, kRootFolderCnid);
        for (int f = 0; f < 5; ++f) {
            writeBE32(&folderRec.data, now);
        }
        writeBE32(&folderRec.data, 0);
        writeBE16(&folderRec.data, 040755);
        writeBE16(&folderRec.data, 0);
        writeBE32(&folderRec.data, 0);
        for (int z = 0; z < 32; ++z) {
            folderRec.data.push_back(0);
        }
        records.push_back(folderRec);
    }

    for (size_t i = 0; i < mItems.size(); ++i) {
        const CatalogItem& item = mItems[i];
        if (item.kind == CatalogItem::Kind::Folder) {
            RawCatalogRecord threadRec;
            appendThreadKey(&threadRec.key, item.cnid);
            appendFolderThreadData(&threadRec.data, item.parentCnid, item.name);
            records.push_back(threadRec);

            RawCatalogRecord folderRec;
            appendCatalogKey(&folderRec.key, item.parentCnid, item.name);
            writeBE16(&folderRec.data, 1);
            writeBE16(&folderRec.data, 0);
            writeBE32(&folderRec.data, 0);
            writeBE32(&folderRec.data, item.cnid);
            for (int f = 0; f < 5; ++f) {
                writeBE32(&folderRec.data, now);
            }
            writeBE32(&folderRec.data, 0);
            writeBE16(&folderRec.data, 040755);
            writeBE16(&folderRec.data, 0);
            writeBE32(&folderRec.data, 0);
            for (int z = 0; z < 32; ++z) {
                folderRec.data.push_back(0);
            }
            records.push_back(folderRec);
        } else {
            RawCatalogRecord threadRec;
            appendThreadKey(&threadRec.key, item.cnid);
            appendFileThreadData(&threadRec.data, item.parentCnid, item.name);
            records.push_back(threadRec);

            RawCatalogRecord fileRec;
            appendCatalogKey(&fileRec.key, item.parentCnid, item.name);
            writeBE16(&fileRec.data, 2);
            writeBE16(&fileRec.data, 0);
            writeBE16(&fileRec.data, 0);
            writeBE32(&fileRec.data, item.cnid);
            for (int f = 0; f < 5; ++f) {
                writeBE32(&fileRec.data, now);
            }
            writeBE32(&fileRec.data, 0);
            writeBE32(&fileRec.data, 0);
            writeBE16(&fileRec.data, item.fileMode);
            writeBE16(&fileRec.data, 0);
            writeBE32(&fileRec.data, 0);
            for (int z = 0; z < 32; ++z) {
                fileRec.data.push_back(0);
            }
            writeBE32(&fileRec.data, 0);
            writeBE32(&fileRec.data, 0);
            appendFork(&fileRec.data, item.data.size(), item.dataStartBlock, item.dataBlockCount);
            appendFork(&fileRec.data, 0, 0, 0);
            records.push_back(fileRec);
        }
    }

    std::sort(records.begin(), records.end(), keyLess);

    std::vector<uint8_t> leaf(blockSize, 0);
    patchBE32(&leaf, 0, 0);
    patchBE32(&leaf, 4, 0);
    leaf[8] = static_cast<uint8_t>(0xff);
    leaf[9] = 0;
    leaf[10] = 0;
    const uint16_t recordCount = static_cast<uint16_t>(records.size());
    patchBE16(&leaf, 10, recordCount);
    patchBE16(&leaf, 12, 0);

    size_t cursor = 14;
    std::vector<uint16_t> offsets;
    offsets.push_back(14);
    for (size_t i = 0; i < records.size(); ++i) {
        if (cursor + records[i].key.size() + records[i].data.size() > blockSize - 2) {
            Logger::error("  [Ramdisk] catalog leaf overflow");
            return false;
        }
        for (size_t k = 0; k < records[i].key.size(); ++k) {
            leaf[cursor++] = records[i].key[k];
        }
        for (size_t d = 0; d < records[i].data.size(); ++d) {
            leaf[cursor++] = records[i].data[d];
        }
        if (i + 1 < records.size()) {
            offsets.push_back(static_cast<uint16_t>(cursor));
        }
    }
    for (size_t i = 0; i < offsets.size(); ++i) {
        const size_t offPos = blockSize - (i + 1) * 2;
        leaf[offPos] = static_cast<uint8_t>((offsets[i] >> 8) & 0xff);
        leaf[offPos + 1] = static_cast<uint8_t>(offsets[i] & 0xff);
    }

    std::vector<uint8_t> header(blockSize, 0);
    patchBE32(&header, 0, 0);
    patchBE32(&header, 4, 0);
    header[8] = 1;
    header[9] = 0;
    patchBE16(&header, 10, 0);
    patchBE16(&header, 12, 0);
    patchBE16(&header, 14, 1);
    patchBE32(&header, 16, 1);
    patchBE32(&header, 20, recordCount);
    patchBE32(&header, 24, 1);
    patchBE32(&header, 28, 1);
    patchBE16(&header, 32, static_cast<uint16_t>(blockSize));
    patchBE16(&header, 34, 520);
    patchBE32(&header, 36, 2);
    patchBE32(&header, 40, 0);
    patchBE16(&header, 44, 0);
    patchBE32(&header, 46, 0);
    header[0x50] = 'H';
    header[0x51] = '+';
    header[0x52] = 'B';
    header[0x53] = 'T';
    header[0x54] = 0;

    const size_t headerOffset = 1024;
    (*image)[headerOffset + 0] = 'H';
    (*image)[headerOffset + 1] = '+';
    patchBE16(image, headerOffset + 2, 4);
    patchBE32(image, headerOffset + 4, 0x5c);
    (*image)[headerOffset + 8] = '8';
    (*image)[headerOffset + 9] = '.';
    (*image)[headerOffset + 10] = '0';
    (*image)[headerOffset + 11] = '0';
    patchBE32(image, headerOffset + 16, now);
    patchBE32(image, headerOffset + 20, now);
    patchBE32(image, headerOffset + 24, now);
    patchBE32(image, headerOffset + 28, now);

    uint32_t fileCount = 0;
    uint32_t folderCount = 1;
    for (size_t i = 0; i < mItems.size(); ++i) {
        if (mItems[i].kind == CatalogItem::Kind::File) {
            ++fileCount;
        } else {
            ++folderCount;
        }
    }
    patchBE32(image, headerOffset + 32, fileCount);
    patchBE32(image, headerOffset + 36, folderCount);
    patchBE32(image, headerOffset + 40, blockSize);
    patchBE32(image, headerOffset + 44, total);
    patchBE32(image, headerOffset + 48, total - nextDataBlock);
    patchBE32(image, headerOffset + 52, nextDataBlock);
    patchBE32(image, headerOffset + 56, blockSize);
    patchBE32(image, headerOffset + 60, blockSize);
    patchBE32(image, headerOffset + 64, mNextCnid);
    patchBE32(image, headerOffset + 68, 0);
    patchBE32(image, headerOffset + 72, 0);
    patchBE32(image, headerOffset + 76, 0);

    const size_t forkBase = headerOffset + 112;
    const uint32_t bitmapBytes = (total + 7) / 8;
    std::vector<uint8_t> forkTmp;
    forkTmp.clear();
    appendFork(&forkTmp, bitmapBytes, kAllocBlock, 1);
    for (size_t i = 0; i < forkTmp.size(); ++i) {
        (*image)[forkBase + i] = forkTmp[i];
    }
    forkTmp.clear();
    appendFork(&forkTmp, 0, kExtentsBlock, 1);
    for (size_t i = 0; i < forkTmp.size(); ++i) {
        (*image)[forkBase + 80 + i] = forkTmp[i];
    }
    forkTmp.clear();
    appendFork(&forkTmp, blockSize * kCatalogBlockCount, kCatalogBlock, kCatalogBlockCount);
    for (size_t i = 0; i < forkTmp.size(); ++i) {
        (*image)[forkBase + 160 + i] = forkTmp[i];
    }

    for (uint32_t b = 0; b < nextDataBlock; ++b) {
        const size_t byteIndex = static_cast<size_t>(b / 8);
        const size_t bitIndex = static_cast<size_t>(7 - (b % 8));
        const size_t allocOffset = static_cast<size_t>(kAllocBlock) * blockSize + byteIndex;
        if (allocOffset < image->size()) {
            (*image)[allocOffset] |= static_cast<uint8_t>(1U << bitIndex);
        }
    }

    const size_t catalogOffset = static_cast<size_t>(kCatalogBlock) * blockSize;
    for (size_t i = 0; i < header.size(); ++i) {
        (*image)[catalogOffset + i] = header[i];
    }
    for (size_t i = 0; i < leaf.size(); ++i) {
        (*image)[catalogOffset + blockSize + i] = leaf[i];
    }

    for (size_t i = 0; i < mItems.size(); ++i) {
        const CatalogItem& item = mItems[i];
        if (item.kind != CatalogItem::Kind::File || item.dataBlockCount == 0) {
            continue;
        }
        const size_t base = static_cast<size_t>(item.dataStartBlock) * blockSize;
        for (size_t b = 0; b < item.data.size(); ++b) {
            if (base + b < image->size()) {
                (*image)[base + b] = item.data[b];
            }
        }
    }

    return true;
}

bool HfsPlusWriter::buildToFile(const std::string& path) {
    std::vector<uint8_t> image;
    if (!build(&image)) {
        return false;
    }
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        Logger::error("  [Ramdisk] failed to open output: " + path);
        return false;
    }
    out.write(reinterpret_cast<const char*>(&image[0]), static_cast<std::streamsize>(image.size()));
    return out.good();
}

} /* namespace PP */
