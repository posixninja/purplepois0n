/*
 * RamdiskTypes.cpp
 */

#include "RamdiskTypes.h"

#include <cctype>
#include <cstdlib>

namespace PP {

namespace {

uint64_t applySizeSuffix(uint64_t value, char suffix) {
    switch (suffix) {
        case 'k':
        case 'K':
            return value * 1024ULL;
        case 'm':
        case 'M':
            return value * 1024ULL * 1024ULL;
        case 'g':
        case 'G':
            return value * 1024ULL * 1024ULL * 1024ULL;
        default:
            return value;
    }
}

} /* anonymous */

RamdiskOptions ramdiskOptionsFromEnv() {
    RamdiskOptions opts;
    const char* label = std::getenv("PURPLEPOIS0N_RAMDISK_LABEL");
    if (label != nullptr && label[0] != '\0') {
        opts.volumeLabel = label;
    }
    const char* size = std::getenv("PURPLEPOIS0N_RAMDISK_SIZE");
    if (size != nullptr && size[0] != '\0') {
        uint64_t bytes = 0;
        if (parseRamdiskSize(size, &bytes)) {
            opts.sizeBytes = bytes;
        }
    }
    return opts;
}

bool parseRamdiskSize(const std::string& text, uint64_t* outBytes) {
    if (outBytes == nullptr || text.empty()) {
        return false;
    }
    size_t i = 0;
    while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) {
        ++i;
    }
    uint64_t value = 0;
    while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
        value = value * 10 + static_cast<uint64_t>(text[i] - '0');
        ++i;
    }
    if (i < text.size()) {
        value = applySizeSuffix(value, text[i]);
    }
    if (value < 4096) {
        return false;
    }
    *outBytes = value;
    return true;
}

bool parseRamdiskAddSpec(const std::string& spec, RamdiskStageEntry* out) {
    if (out == nullptr || spec.empty()) {
        return false;
    }
    const size_t split = spec.rfind(":/");
    if (split == std::string::npos || split + 1 >= spec.size()) {
        return false;
    }
    out->hostPath = spec.substr(0, split);
    out->hfsPath = spec.substr(split + 1);
    if (out->hostPath.empty() || out->hfsPath.empty() || out->hfsPath[0] != '/') {
        return false;
    }
    return true;
}

RamdiskTransport ramdiskTransportFromString(const std::string& text) {
    if (text == "ssh" || text == "SSH") {
        return RamdiskTransport::Ssh;
    }
    return RamdiskTransport::TcpLine;
}

std::string ramdiskTransportName(RamdiskTransport transport) {
    switch (transport) {
        case RamdiskTransport::Ssh:
            return "ssh";
        default:
            return "tcp";
    }
}

uint16_t RamdiskConnectOptions::localForwardPort() const {
    return transport == RamdiskTransport::Ssh ? sshPort : tcpPort;
}

uint16_t RamdiskConnectOptions::deviceForwardPort() const {
    return transport == RamdiskTransport::Ssh ? deviceSshPort : deviceTcpPort;
}

} /* namespace PP */
