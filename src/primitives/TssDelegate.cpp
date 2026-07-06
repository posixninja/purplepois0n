/*
 * TssDelegate.cpp
 */

#include "primitives/TssDelegate.h"
#include "TssClient.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace PP {
namespace primitives {

namespace {

std::vector<std::string> splitArgs(const std::string& text) {
    std::vector<std::string> out;
    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == ' ' || c == '\t') {
            if (!current.empty()) {
                out.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

void appendSplitArgs(std::vector<std::string>* argv, const std::string& text) {
    const std::vector<std::string> parts = splitArgs(text);
    for (size_t i = 0; i < parts.size(); ++i) {
        argv->push_back(parts[i]);
    }
}

} /* anonymous */

std::string TssDelegate::findIdevicerestore() {
    const char* env = std::getenv("PURPLEPOIS0N_IDEVICERESTORE");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }
    return ToolRunner::findExecutable("idevicerestore");
}

std::string TssDelegate::findFuturerestore() {
    const char* env = std::getenv("PURPLEPOIS0N_FUTURERESTORE");
    if (env != nullptr && env[0] != '\0' && access(env, X_OK) == 0) {
        return std::string(env);
    }
    return ToolRunner::findExecutable("futurerestore");
}

std::string TssDelegate::findIpsw() {
    return ToolRunner::findIpswExecutable();
}

bool TssDelegate::isIdevicerestoreConfigured() {
    return !findIdevicerestore().empty();
}

bool TssDelegate::isFuturerestoreConfigured() {
    return !findFuturerestore().empty();
}

bool TssDelegate::isIpswConfigured() {
    return !findIpsw().empty();
}

TssSigningMode TssDelegate::resolveSigningMode(const ExecutionContext& context) {
    const TssSigningMode envMode = tssSigningModeFromEnv();
    if (envMode != TssSigningMode::Auto) {
        return envMode;
    }
    if (!context.apticketPath.empty()) {
        return TssSigningMode::SavedApTicket;
    }
    const FutureRestoreOptions fr = futureRestoreOptionsFromEnv();
    if (!fr.apticketPath.empty()) {
        return TssSigningMode::SavedApTicket;
    }
    return TssSigningMode::StockLive;
}

void TssDelegate::logProcessOverview(const ExecutionContext& context) {
    const TssSigningMode mode = resolveSigningMode(context);

    Logger::info("  [TSS]    idevicerestore: live TSS → personalize iBSS/iBEC/IMG4 for signed restore");
    Logger::info("  [TSS]    futurerestore: wrapper — saved APTicket + SEP/BB from *other* IPSWs");
    Logger::info("  [TSS]      • --latest-sep / --latest-baseband: tickets from currently signed builds");
    Logger::info("  [TSS]      • -s/-m and -b/-p: manual SEP.im4p + BuildManifest for SEP/BB tickets");
    Logger::info("  [TSS]      • -t ticket: required for unsigned target IPSW; -w ApNonce collision");
    Logger::info("  [TSS]      • --no-baseband: Wi‑Fi-only / iPod; --update: OTA-style (not from JB OS)");
    Logger::info("  [TSS]      • Odysseus: --use-pwndfu + libipatcher; Prometheus: generator or -w");
    Logger::info(std::string("  [TSS]    mode: ") + tssSigningModeToString(mode));

    if (isIdevicerestoreConfigured()) {
        Logger::info("  [TSS]    idevicerestore: " + findIdevicerestore());
    } else {
        Logger::warn("  [TSS]    idevicerestore: not found (PURPLEPOIS0N_IDEVICERESTORE)");
    }
    if (isFuturerestoreConfigured()) {
        Logger::info("  [TSS]    futurerestore: " + findFuturerestore());
    } else {
        Logger::warn("  [TSS]    futurerestore: not found (PURPLEPOIS0N_FUTURERESTORE)");
    }
    if (isIpswConfigured()) {
        Logger::info("  [TSS]    ipsw (TSS check): " + findIpsw());
    }
    if (TssClient::isLibtatsuLinked()) {
        Logger::info("  [TSS]    libtatsu: linked (in-tree live TSS from idevicerestore tss.c)");
    } else {
        Logger::info("  [TSS]    libtatsu: not linked — use make LIBTATSU=1 after brew install libtatsu");
    }

    if (!context.ipswPath.empty()) {
        Logger::info("  [TSS]    target IPSW: " + context.ipswPath);
    }
    const std::string ticket = !context.apticketPath.empty()
                                   ? context.apticketPath
                                   : futureRestoreOptionsFromEnv().apticketPath;
    if (!ticket.empty()) {
        Logger::info("  [TSS]    APTicket: " + ticket);
    }

    if (TssClient::isLibtatsuLinked()) {
        TssClient::logFuturerestoreSepBbPlan(context);
    }

    if (mode == TssSigningMode::SavedApTicket) {
        const FutureRestoreOptions fr = context.futureRestore;
        if (fr.latestSep && fr.latestBaseband) {
            Logger::info("  [TSS]    futurerestore plan: latest signed SEP + baseband tickets");
        } else if (!fr.sepPath.empty() || !fr.basebandPath.empty()) {
            Logger::info("  [TSS]    futurerestore plan: manual SEP/baseband + separate manifests");
            if (!fr.sepManifestPath.empty()) {
                Logger::info("  [TSS]      sep manifest: " + fr.sepManifestPath);
            }
            if (!fr.basebandManifestPath.empty()) {
                Logger::info("  [TSS]      bb manifest: " + fr.basebandManifestPath);
            }
        } else if (fr.noBaseband) {
            Logger::info("  [TSS]    futurerestore plan: --no-baseband");
        } else {
            Logger::warn("  [TSS]    futurerestore needs SEP/BB strategy (--latest-* or -s/-m -b/-p)");
        }
    }
}

std::vector<std::string> TssDelegate::buildFuturerestoreArgv(const ExecutionContext& context,
                                                             const std::string& ipswPath) {
    std::vector<std::string> argv;
    const std::string tool = findFuturerestore();
    if (tool.empty() || ipswPath.empty()) {
        return argv;
    }

    FutureRestoreOptions fr = context.futureRestore;
    if (fr.apticketPath.empty()) {
        fr.apticketPath = context.apticketPath;
    }
    if (fr.apticketPath.empty()) {
        fr.apticketPath = futureRestoreOptionsFromEnv().apticketPath;
    }

    argv.push_back(tool);
    if (!fr.extraArgs.empty()) {
        appendSplitArgs(&argv, fr.extraArgs);
    }
    if (fr.debug) {
        argv.push_back("--debug");
    }
    if (fr.exitRecovery) {
        argv.push_back("--exit-recovery");
    }

    std::vector<std::string> tickets;
    if (!fr.apticketPath.empty()) {
        tickets.push_back(fr.apticketPath);
    }
    for (size_t i = 0; i < fr.extraApticketPaths.size(); ++i) {
        tickets.push_back(fr.extraApticketPaths[i]);
    }
    for (size_t i = 0; i < tickets.size(); ++i) {
        argv.push_back("-t");
        argv.push_back(tickets[i]);
    }

    if (fr.updateInstall) {
        argv.push_back("--update");
    }
    if (fr.waitApNonce) {
        argv.push_back("-w");
    }
    if (fr.usePwndfu) {
        argv.push_back("--use-pwndfu");
    }
    if (fr.justBoot) {
        argv.push_back("--just-boot");
        if (!fr.justBootArgs.empty()) {
            appendSplitArgs(&argv, fr.justBootArgs);
        }
    }
    if (fr.latestSep) {
        argv.push_back("--latest-sep");
    } else if (!fr.sepPath.empty()) {
        argv.push_back("-s");
        argv.push_back(fr.sepPath);
        if (!fr.sepManifestPath.empty()) {
            argv.push_back("-m");
            argv.push_back(fr.sepManifestPath);
        }
    }
    if (fr.noBaseband) {
        argv.push_back("--no-baseband");
    } else if (fr.latestBaseband) {
        argv.push_back("--latest-baseband");
    } else if (!fr.basebandPath.empty()) {
        argv.push_back("-b");
        argv.push_back(fr.basebandPath);
        if (!fr.basebandManifestPath.empty()) {
            argv.push_back("-p");
            argv.push_back(fr.basebandManifestPath);
        }
    }
    argv.push_back(ipswPath);
    return argv;
}

std::vector<std::string> TssDelegate::buildIdevicerestoreArgv(const ExecutionContext& context,
                                                              const std::string& ipswPath) {
    std::vector<std::string> argv;
    const std::string tool = findIdevicerestore();
    if (tool.empty() || ipswPath.empty()) {
        return argv;
    }

    IdeviceRestoreOptions ir = ideviceRestoreOptionsFromEnv();
    if (context.ideviceRestore.updateInstall) {
        ir.updateInstall = true;
    }
    if (context.ideviceRestore.debug) {
        ir.debug = true;
    }

    argv.push_back(tool);
    if (ir.debug) {
        argv.push_back("-d");
    }
    if (ir.updateInstall) {
        argv.push_back("-u");
    }
    argv.push_back(ipswPath);
    return argv;
}

PrimitiveResult TssDelegate::checkStillSigned(const ExecutionContext& context) {
    const std::string ipsw = findIpsw();
    if (ipsw.empty()) {
        Logger::warn("  [TSS]    ipsw not found — cannot query Apple signing status");
        return PrimitiveResult::PrerequisitesMissing;
    }

    std::vector<std::string> argv;
    argv.push_back(ipsw);
    argv.push_back("download");
    argv.push_back("tss");
    argv.push_back("--signed");

    if (!context.productType.empty()) {
        argv.push_back("--device");
        argv.push_back(context.productType);
    }
    if (!context.iosVersion.empty()) {
        argv.push_back("--version");
        argv.push_back(context.iosVersion);
    }
    if (context.ecid != 0) {
        argv.push_back("--ecid");
        argv.push_back(std::to_string(context.ecid));
    } else if (context.udid.empty()) {
        argv.push_back("--usb");
    }

    Logger::info("  [TSS]    checking Apple signing status (ipsw download tss --signed)...");
    const CommandResult result = ToolRunner::run(argv);
    if (result.exitCode == 0) {
        Logger::info("  [TSS]    build appears still signed by Apple");
        return PrimitiveResult::Success;
    }
    Logger::warn("  [TSS]    not signed or check failed (exit " + std::to_string(result.exitCode) + ")");
    if (!result.stderrText.empty()) {
        Logger::info("  [TSS]    " + result.stderrText);
    }
    return PrimitiveResult::Failed;
}

PrimitiveResult TssDelegate::probe(const ExecutionContext& context) {
    logProcessOverview(context);

    if (!context.productType.empty() && !context.iosVersion.empty()) {
        return checkStillSigned(context);
    }
    if (context.ecid != 0 && context.deviceState == DeviceState::Recovery) {
        Logger::info("  [TSS]    Recovery mode: set ProductType/iOS via Normal connect for signing check");
    }
    return PrimitiveResult::Success;
}

PrimitiveResult TssDelegate::extractIm4mFromApticket(const std::string& apticketPath,
                                                     const std::string& outputPath) {
    if (apticketPath.empty() || outputPath.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }
    if (TssClient::extractIm4mFromShshPlist(apticketPath, outputPath)) {
        return PrimitiveResult::Success;
    }

    const std::string ipsw = findIpsw();
    if (ipsw.empty()) {
        Logger::warn("  [TSS]    ipsw required for IM4M extract fallback");
        return PrimitiveResult::PrerequisitesMissing;
    }

    std::vector<std::string> argv;
    argv.push_back(ipsw);
    argv.push_back("img4");
    argv.push_back("im4m");
    argv.push_back("extract");
    argv.push_back("--output");
    argv.push_back(outputPath);
    argv.push_back(apticketPath);

    Logger::info("  [TSS]    extracting IM4M from APTicket...");
    const CommandResult result = ToolRunner::run(argv);
    return (result.exitCode == 0) ? PrimitiveResult::Success : PrimitiveResult::Failed;
}

PrimitiveResult TssDelegate::personalizeComponent(const std::string& unsignedComponentPath,
                                                const std::string& manifestPath,
                                                const std::string& outputPath,
                                                const std::string& componentFourcc) {
    const std::string ipsw = findIpsw();
    if (ipsw.empty()) {
        Logger::warn("  [TSS]    ipsw required for IMG4 personalize");
        return PrimitiveResult::PrerequisitesMissing;
    }
    if (unsignedComponentPath.empty() || manifestPath.empty() || outputPath.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }

    std::vector<std::string> argv;
    argv.push_back(ipsw);
    argv.push_back("img4");
    argv.push_back("person");
    argv.push_back("--manifest");
    argv.push_back(manifestPath);
    argv.push_back("--output");
    argv.push_back(outputPath);
    if (!componentFourcc.empty()) {
        argv.push_back("--component");
        argv.push_back(componentFourcc);
    }
    argv.push_back(unsignedComponentPath);

    Logger::info("  [TSS]    personalizing component → " + outputPath);
    const CommandResult result = ToolRunner::run(argv);
    return (result.exitCode == 0) ? PrimitiveResult::Success : PrimitiveResult::Failed;
}

bool TssDelegate::isLibtatsuLinked() {
    return TssClient::isLibtatsuLinked();
}

PrimitiveResult TssDelegate::fetchLiveShsh(const ExecutionContext& context,
                                             const std::string& outputPath) {
    if (outputPath.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }
    if (resolveSigningMode(context) != TssSigningMode::StockLive) {
        Logger::info("  [TSS]    fetchLiveShsh skipped — saved APTicket / futurerestore mode");
        return PrimitiveResult::NotApplicable;
    }
    return TssClient::fetchLiveShsh(context, outputPath) ? PrimitiveResult::Success
                                                         : PrimitiveResult::Failed;
}

PrimitiveResult TssDelegate::fetchLiveIm4m(const ExecutionContext& context,
                                          const std::string& shshPath,
                                          const std::string& im4mPath) {
    if (shshPath.empty() || im4mPath.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }
    return TssClient::fetchLiveIm4m(context, shshPath, im4mPath) ? PrimitiveResult::Success
                                                                 : PrimitiveResult::Failed;
}

PrimitiveResult TssDelegate::runFuturerestoreRestore(const ExecutionContext& context,
                                                     bool allowMutation) {
    if (context.ipswPath.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }

    const std::vector<std::string> argv = buildFuturerestoreArgv(context, context.ipswPath);
    if (argv.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (!allowMutation) {
        Logger::info("  [TSS]    futurerestore command (probe only):");
        std::ostringstream line;
        for (size_t i = 0; i < argv.size(); ++i) {
            if (i > 0) {
                line << ' ';
            }
            line << argv[i];
        }
        Logger::info("  [TSS]      " + line.str());
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [TSS]    futurerestore restore requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    Logger::warn("  [TSS]    spawning futurerestore — destructive restore, user-initiated only");
    const CommandResult result = ToolRunner::run(argv);
    return (result.exitCode == 0) ? PrimitiveResult::Success : PrimitiveResult::Failed;
}

PrimitiveResult TssDelegate::runIdevicerestoreRestore(const ExecutionContext& context,
                                                      bool allowMutation) {
    if (context.ipswPath.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }

    const std::vector<std::string> argv = buildIdevicerestoreArgv(context, context.ipswPath);
    if (argv.empty()) {
        return PrimitiveResult::PrerequisitesMissing;
    }

    if (!allowMutation) {
        Logger::info("  [TSS]    idevicerestore command (probe only):");
        std::ostringstream line;
        for (size_t i = 0; i < argv.size(); ++i) {
            if (i > 0) {
                line << ' ';
            }
            line << argv[i];
        }
        Logger::info("  [TSS]      " + line.str());
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [TSS]    idevicerestore restore requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    Logger::warn("  [TSS]    spawning idevicerestore — destructive restore, user-initiated only");
    const CommandResult result = ToolRunner::run(argv);
    return (result.exitCode == 0) ? PrimitiveResult::Success : PrimitiveResult::Failed;
}

} /* namespace primitives */
} /* namespace PP */
