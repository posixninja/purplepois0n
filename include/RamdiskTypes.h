/*
 * RamdiskTypes.h
 *
 * Host-side custom HFS+ ramdisk options (research / Recovery staging).
 */

#ifndef RAMDISK_TYPES_H_
#define RAMDISK_TYPES_H_

#include <cstdint>
#include <string>
#include <vector>

namespace PP {

struct RamdiskOptions {
    std::string volumeLabel = "purplepois0n";
    uint32_t blockSize = 4096;
    /** Total volume size in bytes (default 16 MiB). */
    uint64_t sizeBytes = 16ULL * 1024ULL * 1024ULL;
};

struct RamdiskFileEntry {
    /** Absolute HFS path (e.g. /files/hello.txt). */
    std::string path;
    std::vector<uint8_t> data;
    uint16_t fileMode = 0100755;
};

/** Host file copied into the HFS+ volume at build time (e.g. arm64 agent from Mac). */
struct RamdiskStageEntry {
    std::string hostPath;
    /** Absolute HFS path (e.g. /sbin/pp-agent). */
    std::string hfsPath;
    uint16_t mode = 0100644;
    /** When true, require arm64 Mach-O and default mode 0755. */
    bool verifyMachOArm64 = false;
};

enum class RamdiskTransport {
    /** Line protocol on a TCP port (default — user-supplied agent, no sshd). */
    TcpLine,
    /** OpenSSH + scp when overlay ships sshd/dropbear. */
    Ssh,
};

/** Load options from PURPLEPOIS0N_RAMDISK_* env. */
RamdiskOptions ramdiskOptionsFromEnv();

/** Parse size strings like 16m, 32M, 65536. */
bool parseRamdiskSize(const std::string& text, uint64_t* outBytes);

/** Parse `HOST_PATH:/hfs/path` for --ramdisk-add. */
bool parseRamdiskAddSpec(const std::string& spec, RamdiskStageEntry* out);

RamdiskTransport ramdiskTransportFromString(const std::string& text);
std::string ramdiskTransportName(RamdiskTransport transport);

/** Live host↔ramdisk I/O after boot (custom overlay agent or SSH). */
struct RamdiskConnectOptions {
    RamdiskTransport transport = RamdiskTransport::TcpLine;
    std::string udid;
    std::string host = "127.0.0.1";
    uint16_t tcpPort = 4444;
    uint16_t deviceTcpPort = 4444;
    uint16_t sshPort = 2222;
    uint16_t deviceSshPort = 22;
    std::string sshUser = "root";
    std::string sshPassword = "alpine";
    std::string sshKeyPath;
    bool sshInsecure = true;
    /** When true and @p udid is set, spawn iproxy for the session. */
    bool autoIproxy = true;

    uint16_t localForwardPort() const;
    uint16_t deviceForwardPort() const;
};

RamdiskConnectOptions ramdiskConnectOptionsFromEnv();

} /* namespace PP */

#endif /* RAMDISK_TYPES_H_ */
