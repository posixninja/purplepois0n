/*
 * RamdiskWorkDir.cpp
 */

#include "RamdiskWorkDir.h"

#include <cstdlib>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace PP {

std::string defaultRamdiskWorkDir() {
    std::ostringstream oss;
    oss << "/tmp/pp-ramdisk-" << static_cast<unsigned long>(getpid());
    return oss.str();
}

std::string resolveRamdiskWorkDir(const std::string& cliPath) {
    if (!cliPath.empty()) {
        return cliPath;
    }
    const char* env = std::getenv("PURPLEPOIS0N_RAMDISK_WORK_DIR");
    if (env != nullptr && env[0] != '\0') {
        return std::string(env);
    }
    return defaultRamdiskWorkDir();
}

bool ensureDirectory(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return mkdir(path.c_str(), 0755) == 0;
}

} /* namespace PP */
