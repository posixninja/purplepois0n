/*
 * CodesignDelegate.cpp
 */

#include "primitives/CodesignDelegate.h"
#include "IpaSignHelper.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace PP {
namespace primitives {

namespace {

void appendSplitArgs(std::vector<std::string>* argv, const std::string& text) {
    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == ' ' || c == '\t') {
            if (!current.empty()) {
                argv->push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        argv->push_back(current);
    }
}

std::string resolveOutputPath(const CodesignOptions& opts, const std::string& inputPath) {
    if (!opts.outputPath.empty()) {
        return opts.outputPath;
    }
    return inputPath;
}

} /* anonymous */

std::string CodesignDelegate::findIpsw() {
    return ToolRunner::findIpswExecutable();
}

std::string CodesignDelegate::findLdid() {
    const char* env = std::getenv("PURPLEPOIS0N_LDID");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }
    return ToolRunner::findExecutable("ldid");
}

std::string CodesignDelegate::findCodesign() {
    return ToolRunner::findExecutable("codesign");
}

bool CodesignDelegate::isIpswConfigured() {
    return !findIpsw().empty();
}

bool CodesignDelegate::isLdidConfigured() {
    return !findLdid().empty();
}

CodesignOptions CodesignDelegate::mergeOptions(const CodesignOptions& cli,
                                               const CodesignOptions& env) {
    CodesignOptions merged = env;
    if (!cli.bundleId.empty()) {
        merged.bundleId = cli.bundleId;
    }
    if (!cli.entitlementsPath.empty()) {
        merged.entitlementsPath = cli.entitlementsPath;
    }
    if (!cli.teamId.empty()) {
        merged.teamId = cli.teamId;
    }
    if (!cli.p12Path.empty()) {
        merged.p12Path = cli.p12Path;
        merged.adHoc = false;
    }
    if (!cli.p12Password.empty()) {
        merged.p12Password = cli.p12Password;
    }
    if (!cli.outputPath.empty()) {
        merged.outputPath = cli.outputPath;
    }
    merged.adHoc = cli.adHoc;
    merged.overwrite = cli.overwrite;
    return merged;
}

void CodesignDelegate::logToolOverview() {
    Logger::info("  [Codesign] host signing for sideload research (not on-device AMFI bypass)");
    if (isIpswConfigured()) {
        Logger::info("  [Codesign] ipsw: " + findIpsw());
    } else {
        Logger::warn("  [Codesign] ipsw not found — make external-ipsw");
    }
    if (isLdidConfigured()) {
        Logger::info("  [Codesign] ldid: " + findLdid());
    }
    if (!findCodesign().empty()) {
        Logger::info("  [Codesign] codesign: " + findCodesign() + " (verify only on macOS)");
    }
    Logger::info("  [Codesign] ad-hoc IPAs need jailbreak + trust cache on device (see TrustCache)");
}

std::vector<std::string> CodesignDelegate::buildIpswSignArgv(const CodesignOptions& opts,
                                                             const std::string& inputPath) {
    std::vector<std::string> argv;
    const std::string ipsw = findIpsw();
    if (ipsw.empty() || inputPath.empty()) {
        return argv;
    }

    argv.push_back(ipsw);
    argv.push_back("macho");
    argv.push_back("sign");

    const char* extra = std::getenv("PURPLEPOIS0N_CODESIGN_ARGS");
    if (extra != nullptr && extra[0] != '\0') {
        appendSplitArgs(&argv, extra);
    }

    if (!opts.bundleId.empty()) {
        argv.push_back("--id");
        argv.push_back(opts.bundleId);
    }
    if (!opts.teamId.empty()) {
        argv.push_back("--team");
        argv.push_back(opts.teamId);
    }
    if (opts.adHoc) {
        argv.push_back("--ad-hoc");
    } else if (!opts.p12Path.empty()) {
        argv.push_back("--cert");
        argv.push_back(opts.p12Path);
        if (!opts.p12Password.empty()) {
            argv.push_back("--pw");
            argv.push_back(opts.p12Password);
        }
    }
    if (!opts.entitlementsPath.empty()) {
        argv.push_back("--ent");
        argv.push_back(opts.entitlementsPath);
    }
    const std::string out = resolveOutputPath(opts, inputPath);
    if (out != inputPath) {
        argv.push_back("--output");
        argv.push_back(out);
    }
    if (opts.overwrite) {
        argv.push_back("--overwrite");
    }
    argv.push_back(inputPath);
    return argv;
}

PrimitiveResult CodesignDelegate::signWithLdid(const CodesignOptions& opts,
                                               const std::string& binaryPath) {
    const std::string ldid = findLdid();
    if (ldid.empty() || binaryPath.empty()) {
        return PrimitiveResult::NotApplicable;
    }

    std::vector<std::string> argv;
    argv.push_back(ldid);
    if (!opts.entitlementsPath.empty()) {
        argv.push_back("-S" + opts.entitlementsPath);
    }
    argv.push_back(binaryPath);

    Logger::info("  [Codesign] ldid sign: " + binaryPath);
    const CommandResult result = ToolRunner::run(argv);
    if (result.exitCode == 0) {
        return PrimitiveResult::Success;
    }
    Logger::warn("  [Codesign] ldid failed (exit " + std::to_string(result.exitCode) + ")");
    if (!result.stderrText.empty()) {
        Logger::info("  [Codesign] " + result.stderrText);
    }
    return PrimitiveResult::Failed;
}

PrimitiveResult CodesignDelegate::signWithIpsw(const CodesignOptions& opts,
                                               const std::string& inputPath) {
    const std::vector<std::string> argv = buildIpswSignArgv(opts, inputPath);
    if (argv.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }

    std::ostringstream line;
    for (size_t i = 0; i < argv.size(); ++i) {
        if (i > 0) {
            line << ' ';
        }
        line << argv[i];
    }
    Logger::info("  [Codesign] " + line.str());

    const CommandResult result = ToolRunner::run(argv);
    if (result.exitCode == 0) {
        Logger::info("  [Codesign] signed → " + resolveOutputPath(opts, inputPath));
        return PrimitiveResult::Success;
    }
    Logger::error("  [Codesign] ipsw macho sign failed (exit " +
                  std::to_string(result.exitCode) + ")");
    if (!result.stderrText.empty()) {
        Logger::info("  [Codesign] " + result.stderrText);
    }
    return PrimitiveResult::Failed;
}

PrimitiveResult CodesignDelegate::signPath(const CodesignOptions& opts,
                                           const std::string& inputPath,
                                           bool allowMutation) {
    if (inputPath.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (!allowMutation) {
        logToolOverview();
        const std::vector<std::string> argv = buildIpswSignArgv(opts, inputPath);
        if (argv.empty()) {
            return PrimitiveResult::PrerequisitesMissing;
        }
        Logger::info("  [Codesign] sign plan (probe only):");
        Logger::info(std::string("  [Codesign]   mode: ") +
                     (opts.adHoc ? "ad-hoc" : "certificate (p12)"));
        std::ostringstream line;
        for (size_t i = 0; i < argv.size(); ++i) {
            if (i > 0) {
                line << ' ';
            }
            line << argv[i];
        }
        Logger::info("  [Codesign]   " + line.str());
        return PrimitiveResult::Success;
    }

    if (isLdidConfigured() && !opts.entitlementsPath.empty()) {
        const PrimitiveResult ldidResult = signWithLdid(opts, inputPath);
        if (ldidResult == PrimitiveResult::Success && isIpswConfigured()) {
            return signWithIpsw(opts, inputPath);
        }
        if (ldidResult == PrimitiveResult::Success) {
            return ldidResult;
        }
    }

    return signWithIpsw(opts, inputPath);
}

PrimitiveResult CodesignDelegate::probe(const ExecutionContext& context) {
    logToolOverview();

    if (!context.codesignInputPath.empty()) {
        CodesignOptions opts = mergeOptions(context.codesign, codesignOptionsFromEnv());
        if (!context.codesignOutputPath.empty()) {
            opts.outputPath = context.codesignOutputPath;
        }
        const std::string& path = context.codesignInputPath;
        if (path.size() > 4 && path.compare(path.size() - 4, 4, ".ipa") == 0) {
            return signIpa(opts, path, opts.outputPath, context.allowMutation).empty()
                       ? PrimitiveResult::Failed
                       : PrimitiveResult::Success;
        }
        return signPath(opts, path, context.allowMutation);
    }
    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
