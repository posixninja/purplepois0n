/*
 * RamdiskClient.h
 *
 * Live host↔ramdisk I/O: TCP line protocol (default) or SSH/SCP.
 */

#ifndef RAMDISK_CLIENT_H_
#define RAMDISK_CLIENT_H_

#include "RamdiskTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PP {

struct RamdiskCommandResult {
    int exitCode = -1;
    std::string stdoutText;
    std::string stderrText;
};

/** Talk to a booted custom ramdisk via user-supplied TCP agent or optional SSH. */
class RamdiskClient {
public:
    explicit RamdiskClient(const RamdiskConnectOptions& options);
    ~RamdiskClient();

    RamdiskClient(const RamdiskClient&) = delete;
    RamdiskClient& operator=(const RamdiskClient&) = delete;

    /** Reachability check (PING/PONG or SSH echo). */
    bool probe(std::string* message);

    RamdiskCommandResult exec(const std::string& command);
    bool uploadFile(const std::string& localPath, const std::string& remotePath);
    bool downloadFile(const std::string& remotePath, const std::string& localPath);
    RamdiskCommandResult listDirectory(const std::string& remotePath);

private:
    RamdiskConnectOptions mOptions;
    bool mIproxyStarted = false;
    int mIproxyPid = -1;

    std::string findSsh() const;
    std::string findScp() const;
    std::string findSshpass() const;
    std::string findIproxy() const;
    bool startIproxy();
    void stopIproxy();
    RamdiskCommandResult runSsh(const std::vector<std::string>& remoteArgs, bool captureOutput);
    bool runScpToRemote(const std::string& localPath, const std::string& remotePath);
    bool runScpFromRemote(const std::string& remotePath, const std::string& localPath);
    void appendSshBaseArgs(std::vector<std::string>* argv) const;

    int tcpConnect(std::string* error) const;
    bool tcpSendLine(int fd, const std::string& line) const;
    bool tcpReadLine(int fd, std::string* line) const;
    bool tcpReadExact(int fd, void* buffer, size_t length) const;
    bool tcpWriteExact(int fd, const void* buffer, size_t length) const;
    bool tcpProbe(std::string* message);
    RamdiskCommandResult tcpExec(const std::string& command);
    bool tcpUploadFile(const std::string& localPath, const std::string& remotePath);
    bool tcpDownloadFile(const std::string& remotePath, const std::string& localPath);
};

} /* namespace PP */

#endif /* RAMDISK_CLIENT_H_ */
