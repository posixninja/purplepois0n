/*
 * ToolRunner.cpp
 */

#include "ToolRunner.h"

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

namespace PP {

namespace {

std::string dirnameOf(const std::string& path) {
    const size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) {
        return std::string();
    }
    return path.substr(0, pos);
}

void appendPathDir(std::string* pathEnv, const std::string& dir) {
    if (dir.empty()) {
        return;
    }
    if (!pathEnv->empty()) {
        *pathEnv += ':';
    }
    *pathEnv += dir;
}

} /* anonymous */

std::string ToolRunner::findIpswExecutable() {
    const char* env = std::getenv("PURPLEPOIS0N_IPSW");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }

    static const char* kBundledCandidates[] = {
        "external/ipsw/ipsw",
        "../external/ipsw/ipsw",
    };
    for (size_t i = 0; i < sizeof(kBundledCandidates) / sizeof(kBundledCandidates[0]); ++i) {
        const std::string candidate = kBundledCandidates[i];
        if (access(candidate.c_str(), X_OK) == 0) {
            return candidate;
        }
    }

    return findExecutable("ipsw");
}

std::string ToolRunner::findExecutable(const char* name) {
    const char* pathEnv = std::getenv("PATH");
    if (pathEnv == nullptr) {
        return std::string();
    }

    std::istringstream stream(pathEnv);
    std::string dir;
    while (std::getline(stream, dir, ':')) {
        if (dir.empty()) {
            continue;
        }
        const std::string candidate = dir + '/' + name;
        if (access(candidate.c_str(), X_OK) == 0) {
            return candidate;
        }
    }
    return std::string();
}

CommandResult ToolRunner::run(const std::vector<std::string>& argv) {
    CommandResult result;
    if (argv.empty()) {
        result.stderrText = "empty argv";
        return result;
    }

    int stdoutPipe[2] = {-1, -1};
    int stderrPipe[2] = {-1, -1};
    if (pipe(stdoutPipe) != 0 || pipe(stderrPipe) != 0) {
        result.stderrText = "pipe failed";
        return result;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stderrPipe[0]);
        close(stderrPipe[1]);
        result.stderrText = "fork failed";
        return result;
    }

    if (pid == 0) {
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stderrPipe[0]);
        close(stderrPipe[1]);

        std::vector<char*> cargv;
        cargv.reserve(argv.size() + 1);
        for (size_t i = 0; i < argv.size(); ++i) {
            cargv.push_back(const_cast<char*>(argv[i].c_str()));
        }
        cargv.push_back(nullptr);

        std::string pathEnv;
        const char* existing = std::getenv("PATH");
        if (existing != nullptr) {
            pathEnv = existing;
        }
        appendPathDir(&pathEnv, dirnameOf(argv[0]));
        if (!pathEnv.empty()) {
            setenv("PATH", pathEnv.c_str(), 1);
        }

        execv(argv[0].c_str(), cargv.data());
        _exit(127);
    }

    close(stdoutPipe[1]);
    close(stderrPipe[1]);

    auto drain = [](int fd) -> std::string {
        std::string out;
        char buffer[4096];
        ssize_t n = 0;
        while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
            out.append(buffer, static_cast<size_t>(n));
        }
        return out;
    };

    result.stdoutText = drain(stdoutPipe[0]);
    result.stderrText = drain(stderrPipe[0]);
    close(stdoutPipe[0]);
    close(stderrPipe[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        result.exitCode = -1;
        result.stderrText += "waitpid failed";
        return result;
    }

    if (WIFEXITED(status)) {
        result.exitCode = WEXITSTATUS(status);
    } else {
        result.exitCode = -1;
    }
    return result;
}

} /* namespace PP */
