/*
 * RamdiskClient.cpp
 */

#include "RamdiskClient.h"
#include "EnvUtil.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <cstdlib>
#include <sstream>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <vector>
#include <sys/socket.h>
#include <netdb.h>

namespace PP {

RamdiskConnectOptions ramdiskConnectOptionsFromEnv() {
    RamdiskConnectOptions opts;
    const char* host = std::getenv("PURPLEPOIS0N_RAMDISK_HOST");
    if (host == nullptr || host[0] == '\0') {
        host = std::getenv("PURPLEPOIS0N_RAMDISK_SSH_HOST");
    }
    if (host != nullptr && host[0] != '\0') {
        opts.host = host;
    }
    const char* transport = std::getenv("PURPLEPOIS0N_RAMDISK_TRANSPORT");
    if (transport != nullptr && transport[0] != '\0') {
        opts.transport = ramdiskTransportFromString(transport);
    }
    opts.tcpPort = parsePortEnv("PURPLEPOIS0N_RAMDISK_TCP_PORT", opts.tcpPort);
    opts.deviceTcpPort =
        parsePortEnv("PURPLEPOIS0N_RAMDISK_DEVICE_TCP_PORT", opts.deviceTcpPort);
    opts.sshPort = parsePortEnv("PURPLEPOIS0N_RAMDISK_SSH_PORT", opts.sshPort);
    opts.deviceSshPort = parsePortEnv("PURPLEPOIS0N_RAMDISK_DEVICE_SSH_PORT", opts.deviceSshPort);
    const char* devicePort = std::getenv("PURPLEPOIS0N_RAMDISK_DEVICE_PORT");
    if (devicePort != nullptr && devicePort[0] != '\0') {
        const long parsed = std::strtol(devicePort, nullptr, 10);
        if (parsed > 0 && parsed <= 65535) {
            opts.deviceTcpPort = static_cast<uint16_t>(parsed);
            opts.deviceSshPort = static_cast<uint16_t>(parsed);
        }
    }
    const char* user = std::getenv("PURPLEPOIS0N_RAMDISK_SSH_USER");
    if (user != nullptr && user[0] != '\0') {
        opts.sshUser = user;
    }
    const char* pass = std::getenv("PURPLEPOIS0N_RAMDISK_SSH_PASS");
    if (pass != nullptr) {
        opts.sshPassword = pass;
    }
    const char* key = std::getenv("PURPLEPOIS0N_RAMDISK_SSH_KEY");
    if (key != nullptr && key[0] != '\0') {
        opts.sshKeyPath = key;
    }
    opts.sshInsecure = truthyEnv("PURPLEPOIS0N_RAMDISK_SSH_INSECURE", opts.sshInsecure);
    opts.autoIproxy = truthyEnv("PURPLEPOIS0N_RAMDISK_AUTO_IPROXY", opts.autoIproxy);
    const char* udid = std::getenv("PURPLEPOIS0N_RAMDISK_UDID");
    if (udid != nullptr && udid[0] != '\0') {
        opts.udid = udid;
    }
    return opts;
}

RamdiskClient::RamdiskClient(const RamdiskConnectOptions& options) : mOptions(options) {
    if (mOptions.autoIproxy && !mOptions.udid.empty()) {
        startIproxy();
    }
}

RamdiskClient::~RamdiskClient() {
    stopIproxy();
}

std::string RamdiskClient::findSsh() const {
    const char* env = std::getenv("PURPLEPOIS0N_SSH");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }
    return ToolRunner::findExecutable("ssh");
}

std::string RamdiskClient::findScp() const {
    const char* env = std::getenv("PURPLEPOIS0N_SCP");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }
    return ToolRunner::findExecutable("scp");
}

std::string RamdiskClient::findSshpass() const {
    const char* env = std::getenv("PURPLEPOIS0N_SSHPASS");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }
    return ToolRunner::findExecutable("sshpass");
}

std::string RamdiskClient::findIproxy() const {
    const char* env = std::getenv("PURPLEPOIS0N_IPROXY");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }
    return ToolRunner::findExecutable("iproxy");
}

bool RamdiskClient::startIproxy() {
    if (mIproxyStarted) {
        return true;
    }
    const std::string iproxy = findIproxy();
    if (iproxy.empty()) {
        Logger::warn("  [Ramdisk] iproxy not found — forward port manually or set PURPLEPOIS0N_IPROXY");
        return false;
    }
    const pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        std::ostringstream mapping;
        mapping << mOptions.localForwardPort() << ":" << mOptions.deviceForwardPort();
        if (!mOptions.udid.empty()) {
            execl(iproxy.c_str(), iproxy.c_str(), "-u", mOptions.udid.c_str(),
                  mapping.str().c_str(), static_cast<char*>(nullptr));
        } else {
            execl(iproxy.c_str(), iproxy.c_str(), mapping.str().c_str(),
                  static_cast<char*>(nullptr));
        }
        _exit(127);
    }
    mIproxyPid = static_cast<int>(pid);
    mIproxyStarted = true;
    usleep(400000);
    Logger::info("  [Ramdisk] iproxy " + std::to_string(mOptions.localForwardPort()) + " → device:" +
                 std::to_string(mOptions.deviceForwardPort()) + " (pid " + std::to_string(mIproxyPid) +
                 ")");
    return true;
}

void RamdiskClient::stopIproxy() {
    if (mIproxyPid > 0) {
        kill(static_cast<pid_t>(mIproxyPid), SIGTERM);
        mIproxyPid = -1;
        mIproxyStarted = false;
    }
}

void RamdiskClient::appendSshBaseArgs(std::vector<std::string>* argv) const {
    if (argv == nullptr) {
        return;
    }
    argv->push_back("-p");
    argv->push_back(std::to_string(mOptions.sshPort));
    argv->push_back("-o");
    argv->push_back("BatchMode=no");
    if (mOptions.sshInsecure) {
        argv->push_back("-o");
        argv->push_back("StrictHostKeyChecking=no");
        argv->push_back("-o");
        argv->push_back("UserKnownHostsFile=/dev/null");
    }
    if (!mOptions.sshKeyPath.empty()) {
        argv->push_back("-i");
        argv->push_back(mOptions.sshKeyPath);
    }
}

RamdiskCommandResult RamdiskClient::runSsh(const std::vector<std::string>& remoteArgs,
                                           bool /*captureOutput*/) {
    RamdiskCommandResult result;
    const std::string ssh = findSsh();
    if (ssh.empty()) {
        result.stderrText = "ssh not found";
        return result;
    }

    std::vector<std::string> argv;
    const std::string sshpass = findSshpass();
    const bool useSshpass = mOptions.sshKeyPath.empty() && !sshpass.empty();
    if (useSshpass) {
        argv.push_back("/usr/bin/env");
        argv.push_back("SSHPASS=" + mOptions.sshPassword);
        argv.push_back(sshpass);
        argv.push_back("-e");
    }
    argv.push_back(ssh);
    appendSshBaseArgs(&argv);
    argv.push_back(mOptions.sshUser + "@" + mOptions.host);
    for (size_t i = 0; i < remoteArgs.size(); ++i) {
        argv.push_back(remoteArgs[i]);
    }

    const CommandResult cmd = ToolRunner::run(argv);
    result.exitCode = cmd.exitCode;
    result.stdoutText = cmd.stdoutText;
    result.stderrText = cmd.stderrText;
    return result;
}

bool RamdiskClient::probe(std::string* message) {
    if (mOptions.transport == RamdiskTransport::TcpLine) {
        return tcpProbe(message);
    }
    if (findSsh().empty()) {
        if (message != nullptr) {
            *message = "ssh not found on PATH (use --ramdisk-transport tcp for custom agent)";
        }
        return false;
    }
    const RamdiskCommandResult echo = runSsh(std::vector<std::string>{"echo", "purplepois0n"}, true);
    if (echo.exitCode != 0) {
        if (message != nullptr) {
            *message = echo.stderrText.empty() ? "SSH probe failed" : echo.stderrText;
        }
        return false;
    }
    if (message != nullptr) {
        *message = "SSH reachable at " + mOptions.host + ":" + std::to_string(mOptions.sshPort);
    }
    return true;
}

RamdiskCommandResult RamdiskClient::exec(const std::string& command) {
    if (mOptions.transport == RamdiskTransport::TcpLine) {
        return tcpExec(command);
    }
    return runSsh(std::vector<std::string>{"sh", "-c", command}, true);
}

bool RamdiskClient::runScpToRemote(const std::string& localPath, const std::string& remotePath) {
    const std::string scp = findScp();
    if (scp.empty()) {
        Logger::error("  [Ramdisk] scp not found");
        return false;
    }
    std::vector<std::string> argv;
    const std::string sshpass = findSshpass();
    if (!mOptions.sshKeyPath.empty() || sshpass.empty()) {
        argv.push_back(scp);
    } else {
        argv.push_back("/usr/bin/env");
        argv.push_back("SSHPASS=" + mOptions.sshPassword);
        argv.push_back(sshpass);
        argv.push_back("-e");
        argv.push_back(scp);
    }
    argv.push_back("-P");
    argv.push_back(std::to_string(mOptions.sshPort));
    if (mOptions.sshInsecure) {
        argv.push_back("-o");
        argv.push_back("StrictHostKeyChecking=no");
        argv.push_back("-o");
        argv.push_back("UserKnownHostsFile=/dev/null");
    }
    if (!mOptions.sshKeyPath.empty()) {
        argv.push_back("-i");
        argv.push_back(mOptions.sshKeyPath);
    }
    argv.push_back(localPath);
    argv.push_back(mOptions.sshUser + "@" + mOptions.host + ":" + remotePath);
    return ToolRunner::run(argv).exitCode == 0;
}

bool RamdiskClient::runScpFromRemote(const std::string& remotePath, const std::string& localPath) {
    const std::string scp = findScp();
    if (scp.empty()) {
        Logger::error("  [Ramdisk] scp not found");
        return false;
    }
    std::vector<std::string> argv;
    const std::string sshpass = findSshpass();
    if (!mOptions.sshKeyPath.empty() || sshpass.empty()) {
        argv.push_back(scp);
    } else {
        argv.push_back("/usr/bin/env");
        argv.push_back("SSHPASS=" + mOptions.sshPassword);
        argv.push_back(sshpass);
        argv.push_back("-e");
        argv.push_back(scp);
    }
    argv.push_back("-P");
    argv.push_back(std::to_string(mOptions.sshPort));
    if (mOptions.sshInsecure) {
        argv.push_back("-o");
        argv.push_back("StrictHostKeyChecking=no");
        argv.push_back("-o");
        argv.push_back("UserKnownHostsFile=/dev/null");
    }
    if (!mOptions.sshKeyPath.empty()) {
        argv.push_back("-i");
        argv.push_back(mOptions.sshKeyPath);
    }
    argv.push_back(mOptions.sshUser + "@" + mOptions.host + ":" + remotePath);
    argv.push_back(localPath);
    return ToolRunner::run(argv).exitCode == 0;
}

bool RamdiskClient::uploadFile(const std::string& localPath, const std::string& remotePath) {
    Logger::info("  [Ramdisk] upload " + localPath + " → " + remotePath);
    if (mOptions.transport == RamdiskTransport::TcpLine) {
        return tcpUploadFile(localPath, remotePath);
    }
    return runScpToRemote(localPath, remotePath);
}

bool RamdiskClient::downloadFile(const std::string& remotePath, const std::string& localPath) {
    Logger::info("  [Ramdisk] download " + remotePath + " → " + localPath);
    if (mOptions.transport == RamdiskTransport::TcpLine) {
        return tcpDownloadFile(remotePath, localPath);
    }
    return runScpFromRemote(remotePath, localPath);
}

RamdiskCommandResult RamdiskClient::listDirectory(const std::string& remotePath) {
    const std::string path = remotePath.empty() ? std::string("/") : remotePath;
    if (mOptions.transport == RamdiskTransport::TcpLine) {
        return tcpExec("ls -la " + path);
    }
    return runSsh(std::vector<std::string>{"ls", "-la", path}, true);
}

int RamdiskClient::tcpConnect(std::string* error) const {
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* res = nullptr;
    const std::string portStr = std::to_string(mOptions.localForwardPort());
    if (getaddrinfo(mOptions.host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        if (error != nullptr) {
            *error = "getaddrinfo failed for " + mOptions.host;
        }
        return -1;
    }
    int fd = -1;
    for (struct addrinfo* node = res; node != nullptr; node = node->ai_next) {
        fd = static_cast<int>(socket(node->ai_family, node->ai_socktype, node->ai_protocol));
        if (fd < 0) {
            continue;
        }
        if (connect(fd, node->ai_addr, node->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0 && error != nullptr) {
        *error = "TCP connect failed at " + mOptions.host + ":" + portStr;
    }
    return fd;
}

bool RamdiskClient::tcpWriteExact(int fd, const void* buffer, size_t length) const {
    const char* bytes = static_cast<const char*>(buffer);
    size_t sent = 0;
    while (sent < length) {
        const ssize_t chunk = send(fd, bytes + sent, length - sent, 0);
        if (chunk <= 0) {
            return false;
        }
        sent += static_cast<size_t>(chunk);
    }
    return true;
}

bool RamdiskClient::tcpReadExact(int fd, void* buffer, size_t length) const {
    char* bytes = static_cast<char*>(buffer);
    size_t received = 0;
    while (received < length) {
        const ssize_t chunk = recv(fd, bytes + received, length - received, 0);
        if (chunk <= 0) {
            return false;
        }
        received += static_cast<size_t>(chunk);
    }
    return true;
}

bool RamdiskClient::tcpSendLine(int fd, const std::string& line) const {
    return tcpWriteExact(fd, line.data(), line.size()) && tcpWriteExact(fd, "\n", 1);
}

bool RamdiskClient::tcpReadLine(int fd, std::string* line) const {
    if (line == nullptr) {
        return false;
    }
    line->clear();
    char byte = '\0';
    while (true) {
        if (!tcpReadExact(fd, &byte, 1)) {
            return false;
        }
        if (byte == '\n') {
            break;
        }
        if (byte != '\r') {
            line->push_back(byte);
        }
    }
    return true;
}

bool RamdiskClient::tcpProbe(std::string* message) {
    std::string error;
    const int fd = tcpConnect(&error);
    if (fd < 0) {
        if (message != nullptr) {
            *message = error;
        }
        return false;
    }
    bool ok = false;
    std::string reply;
    if (tcpSendLine(fd, "PING") && tcpReadLine(fd, &reply) && reply == "PONG") {
        ok = true;
        if (message != nullptr) {
            *message = "TCP agent at " + mOptions.host + ":" +
                       std::to_string(mOptions.localForwardPort());
        }
    } else if (message != nullptr) {
        *message = reply.empty() ? "TCP agent did not reply PONG" : ("unexpected: " + reply);
    }
    close(fd);
    return ok;
}

RamdiskCommandResult RamdiskClient::tcpExec(const std::string& command) {
    RamdiskCommandResult result;
    std::string error;
    const int fd = tcpConnect(&error);
    if (fd < 0) {
        result.stderrText = error;
        return result;
    }
    if (!tcpSendLine(fd, "EXEC " + command)) {
        close(fd);
        result.stderrText = "TCP send failed";
        return result;
    }
    std::string status;
    if (!tcpReadLine(fd, &status)) {
        close(fd);
        result.stderrText = "TCP read failed";
        return result;
    }
    if (status.rfind("ERR ", 0) == 0) {
        close(fd);
        result.stderrText = status.substr(4);
        result.exitCode = 1;
        return result;
    }
    if (status.rfind("OK ", 0) != 0) {
        close(fd);
        result.stderrText = "unexpected agent reply: " + status;
        return result;
    }
    result.exitCode = std::atoi(status.c_str() + 3);
    std::ostringstream body;
    while (true) {
        std::string line;
        if (!tcpReadLine(fd, &line)) {
            close(fd);
            result.stderrText = "TCP read body failed";
            result.exitCode = -1;
            return result;
        }
        if (line == ".") {
            break;
        }
        body << line << '\n';
    }
    close(fd);
    result.stdoutText = body.str();
    return result;
}

bool RamdiskClient::tcpUploadFile(const std::string& localPath, const std::string& remotePath) {
    std::ifstream in(localPath.c_str(), std::ios::binary);
    if (!in.is_open()) {
        Logger::error("  [Ramdisk] cannot read local file: " + localPath);
        return false;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::string error;
    const int fd = tcpConnect(&error);
    if (fd < 0) {
        Logger::error("  [Ramdisk] " + error);
        return false;
    }
    const std::string header =
        "PUT " + remotePath + " " + std::to_string(static_cast<unsigned long long>(data.size()));
    bool ok = tcpSendLine(fd, header);
    if (ok && !data.empty()) {
        ok = tcpWriteExact(fd, &data[0], data.size());
    }
    std::string reply;
    if (ok) {
        ok = tcpReadLine(fd, &reply) && reply == "OK";
    }
    if (!ok) {
        Logger::error("  [Ramdisk] TCP PUT failed" + (reply.empty() ? "" : (": " + reply)));
    }
    close(fd);
    return ok;
}

bool RamdiskClient::tcpDownloadFile(const std::string& remotePath, const std::string& localPath) {
    std::string error;
    const int fd = tcpConnect(&error);
    if (fd < 0) {
        Logger::error("  [Ramdisk] " + error);
        return false;
    }
    bool ok = tcpSendLine(fd, "GET " + remotePath);
    std::string reply;
    if (ok) {
        ok = tcpReadLine(fd, &reply);
    }
    if (!ok) {
        close(fd);
        Logger::error("  [Ramdisk] TCP GET failed");
        return false;
    }
    if (reply.rfind("ERR ", 0) == 0) {
        close(fd);
        Logger::error("  [Ramdisk] " + reply.substr(4));
        return false;
    }
    if (reply.rfind("DATA ", 0) != 0) {
        close(fd);
        Logger::error("  [Ramdisk] unexpected GET reply: " + reply);
        return false;
    }
    const size_t size = static_cast<size_t>(std::strtoul(reply.c_str() + 5, nullptr, 10));
    std::vector<uint8_t> data(size);
    if (size > 0 && !tcpReadExact(fd, &data[0], size)) {
        close(fd);
        Logger::error("  [Ramdisk] TCP GET body read failed");
        return false;
    }
    close(fd);
    std::ofstream out(localPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        Logger::error("  [Ramdisk] cannot write: " + localPath);
        return false;
    }
    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(&data[0]), static_cast<std::streamsize>(data.size()));
    }
    return out.good();
}

} /* namespace PP */
