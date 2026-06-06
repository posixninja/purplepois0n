/*
 * RamdiskWorkDir.h
 */

#ifndef RAMDISK_WORK_DIR_H_
#define RAMDISK_WORK_DIR_H_

#include <string>

namespace PP {

std::string defaultRamdiskWorkDir();
std::string resolveRamdiskWorkDir(const std::string& cliPath);
bool ensureDirectory(const std::string& path);

} /* namespace PP */

#endif /* RAMDISK_WORK_DIR_H_ */
