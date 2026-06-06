/*
 * RamdiskInspector.cpp
 */

#include "RamdiskInspector.h"

#include <fstream>

namespace PP {

bool ramdiskLooksLikeHfsPlus(const std::string& path) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    in.seekg(1024, std::ios::beg);
    char sig[2] = {0, 0};
    in.read(sig, 2);
    return sig[0] == 'H' && sig[1] == '+';
}

} /* namespace PP */
