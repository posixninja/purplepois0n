/*
 * RamdiskInspector.h
 */

#ifndef RAMDISK_INSPECTOR_H_
#define RAMDISK_INSPECTOR_H_

#include <string>

namespace PP {

/** Returns true when byte offset 1024 has H+ signature. */
bool ramdiskLooksLikeHfsPlus(const std::string& path);

} /* namespace PP */

#endif /* RAMDISK_INSPECTOR_H_ */
