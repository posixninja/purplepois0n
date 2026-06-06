/*
 * RamdiskBuilder.cpp
 */

#include "RamdiskBuilder.h"

#include "Logger.h"

#include <dirent.h>
#include <fstream>
#include <sys/stat.h>

namespace PP {

RamdiskBuilder::RamdiskBuilder(const RamdiskOptions& options) : mOptions(options), mWriter(options) {}

std::string RamdiskBuilder::overlayPathToHfs(const std::string& overlayRoot,
                                             const std::string& filePath) {
    if (filePath.size() <= overlayRoot.size()) {
        return std::string();
    }
    std::string rel = filePath.substr(overlayRoot.size());
    while (!rel.empty() && rel[0] == '/') {
        rel.erase(0, 1);
    }
    if (rel.empty()) {
        return std::string();
    }
    return std::string("/") + rel;
}

bool RamdiskBuilder::addOverlayDirectory(const std::string& overlayRoot) {
    if (overlayRoot.empty()) {
        return false;
    }

    std::string stack[512];
    size_t depth = 0;
    stack[depth++] = overlayRoot;

    while (depth > 0) {
        const std::string dir = stack[--depth];
        DIR* d = opendir(dir.c_str());
        if (d == nullptr) {
            Logger::warn("  [Ramdisk] cannot open overlay dir: " + dir);
            continue;
        }
        struct dirent* entry = nullptr;
        while ((entry = readdir(d)) != nullptr) {
            const std::string name = entry->d_name;
            if (name == "." || name == "..") {
                continue;
            }
            const std::string full = dir + "/" + name;
            struct stat st;
            if (stat(full.c_str(), &st) != 0) {
                continue;
            }
            if (S_ISDIR(st.st_mode)) {
                if (depth < sizeof(stack) / sizeof(stack[0])) {
                    stack[depth++] = full;
                }
            } else if (S_ISREG(st.st_mode)) {
                std::ifstream in(full.c_str(), std::ios::binary);
                if (!in.is_open()) {
                    Logger::warn("  [Ramdisk] skip unreadable: " + full);
                    continue;
                }
                std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                                          std::istreambuf_iterator<char>());
                const std::string hfsPath = overlayPathToHfs(overlayRoot, full);
                if (hfsPath.empty() || !mWriter.addFile(hfsPath, data)) {
                    Logger::warn("  [Ramdisk] skip: " + full);
                }
            }
        }
        closedir(d);
    }
    return true;
}

bool RamdiskBuilder::addFile(const std::string& absolutePath, const std::vector<uint8_t>& data,
                             uint16_t fileMode) {
    return mWriter.addFile(absolutePath, data, fileMode);
}

bool RamdiskBuilder::build(std::vector<uint8_t>* image) {
    return mWriter.build(image);
}

bool RamdiskBuilder::buildToFile(const std::string& path) {
    return mWriter.buildToFile(path);
}

} /* namespace PP */
