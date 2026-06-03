/*
 * ToolRunner.h — run external CLI tools (ipsw, etc.) and capture output.
 */

#ifndef TOOL_RUNNER_H_
#define TOOL_RUNNER_H_

#include <string>
#include <vector>

namespace PP {

struct CommandResult {
    int exitCode = -1;
    std::string stdoutText;
    std::string stderrText;
};

class ToolRunner {
public:
    /** Resolve executable on PATH, or empty if not found. */
    static std::string findExecutable(const char* name);

    /**
     * Resolve ipsw binary: PURPLEPOIS0N_IPSW, then external/ipsw/ipsw submodule
     * build, then PATH.
     */
    static std::string findIpswExecutable();

    /** Run argv[0] with remaining args; no shell. */
    static CommandResult run(const std::vector<std::string>& argv);
};

} /* namespace PP */

#endif /* TOOL_RUNNER_H_ */
