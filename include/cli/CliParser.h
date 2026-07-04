/*
 * CliParser.h
 *
 * Subcommand CLI for purplepois0n (device, jailbreak, store, post-jb, analyze, dev).
 */

#ifndef CLI_PARSER_H_
#define CLI_PARSER_H_

#include <string>
#include <vector>

namespace PP {
namespace cli {

/** Result of subcommand pre-parse. */
struct SubcommandResult {
    bool handled = false;       /** true if caller should return exitCode immediately */
    int exitCode = 0;
    bool useLegacy = false;     /** true: run legacy flag parser on rewrittenArgv */
    std::vector<std::string> rewrittenArgv;
    bool showSubcommandHelp = false;
    std::string subcommandHelpTopic;
};

/** Try subcommand dispatch; returns handled=true for help-only exits. */
SubcommandResult trySubcommand(int argc, char* argv[]);

void printSubcommandHelp(const char* programName);
void printTopicHelp(const char* programName, const char* topic);
void printDevHelp(const char* programName);

} /* namespace cli */
} /* namespace PP */

#endif /* CLI_PARSER_H_ */
