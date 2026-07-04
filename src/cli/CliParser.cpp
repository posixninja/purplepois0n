/*
 * CliParser.cpp
 *
 * Subcommand CLI — rewrites to legacy flags for minimal churn in purplepois0n.cpp.
 */

#include "cli/CliParser.h"
#include "Logger.h"

#include <cstring>
#include <iostream>

namespace PP {
namespace cli {

namespace {

bool isFlag(const char* arg) {
    return arg != nullptr && arg[0] == '-';
}

void warnDeprecated(const char* oldFlag, const char* replacement) {
    Logger::warn(std::string("Deprecated flag ") + oldFlag + " — use: " + replacement);
}

void appendRewritten(std::vector<std::string>* out, const char* program, const std::vector<const char*>& flags,
                     int argc, char* argv[], int startIdx) {
    out->push_back(program);
    for (const char* flag : flags) {
        out->push_back(flag);
    }
    for (int i = startIdx; i < argc; ++i) {
        const char* arg = argv[i];
        if (strcmp(arg, "--doctor-run") == 0) {
            warnDeprecated("--doctor-run", "purplepois0n jailbreak");
            out->push_back("--doctor-run");
        } else if (strcmp(arg, "--store-add") == 0) {
            warnDeprecated("--store-add", "purplepois0n store add");
            out->push_back("--store-add");
        } else if (strcmp(arg, "--post-jb-pipeline") == 0) {
            warnDeprecated("--post-jb-pipeline", "purplepois0n post-jb");
            out->push_back("--post-jb-pipeline");
        } else {
            out->push_back(arg);
        }
    }
}

} /* anonymous */

void printTopicHelp(const char* program, const char* topic) {
    if (topic == nullptr || strcmp(topic, "store") == 0) {
        std::cout << "Usage: " << program << " store init|build|add|sync|install|publish [options]\n"
                  << "  init              Initialize repo layout\n"
                  << "  build             Build Packages + Release\n"
                  << "  add DEB           Add package to pool\n"
                  << "  sync              Push repo to device (--normal-ssh -d UDID)\n"
                  << "  install PKG       apt install on device\n"
                  << "  publish [PATH]    Export for HTTPS hosting\n"
                  << "Options: --store-root PATH, -d UDID, --normal-ssh\n";
        return;
    }
    if (strcmp(topic, "device") == 0) {
        std::cout << "Usage: " << program << " device list|info|plan [-d UDID]\n"
                  << "  list              List connected devices (Normal / Recovery / DFU)\n"
                  << "  info              Same as list with lockdown metadata\n"
                  << "  plan              JSON device profile + jailbreak strategy (probe only)\n";
        return;
    }
    if (strcmp(topic, "jailbreak") == 0) {
        std::cout << "Usage: " << program << " jailbreak [--execute] [-d UDID]\n"
                  << "       " << program << " device plan [-d UDID]     Probe-only plan JSON\n"
                  << "       " << program << " external-jailbreak [--already-jailbroken] [--post-jb-store]\n"
                  << "       " << program << " dfu-jailbreak [--i-understand-jailbreak]\n"
                  << "\n"
                  << "  jailbreak         Doctor scan → plan → optional execute (NDJSON on stdout)\n"
                  << "  --execute         Merge planner options and run mutating jailbreak path\n";
        return;
    }
    if (strcmp(topic, "post-jb") == 0) {
        std::cout << "Usage: " << program << " post-jb [--store] [--store-install PKG] [--install-ipa PATH]\n"
                  << "              [--medicine] [--normal-ssh] [-d UDID]\n";
        return;
    }
    if (strcmp(topic, "analyze") == 0) {
        std::cout << "Usage: " << program << " analyze backup|binary|dyldcache|crash PATH\n";
        return;
    }
    printSubcommandHelp(program);
}

void printSubcommandHelp(const char* programName) {
    std::cout << "purplepois0n — iOS jailbreak research framework\n\n"
              << "Usage: " << programName << " <command> [options]\n\n"
              << "Commands:\n"
              << "  device list|info|plan     List devices or print jailbreak plan JSON\n"
              << "  jailbreak [--execute]     Doctor / probe flow (or execute with plugins)\n"
              << "  external-jailbreak        palera1n/checkra1n delegate (recommended)\n"
              << "  dfu-jailbreak             checkm8 + Pongo (DFU, experimental)\n"
              << "  store init|build|add|...  Host apt repo operations\n"
              << "  post-jb                   Post-jailbreak pipeline (IPA, medicine, store)\n"
              << "  analyze backup|binary|... Offline analysis\n"
              << "  capabilities              Print JSON host capabilities\n"
              << "  dev                       Advanced flags (ramdisk, pongo, TSS, …)\n\n"
              << "Host UIs: make agent && make web-dev\n"
              << "Full flag reference: " << programName << " dev --help\n";
}

void printDevHelp(const char* programName) {
    std::cout << "Run: " << programName << " --help\n"
              << "(dev mode exposes all legacy --flags)\n";
}

SubcommandResult trySubcommand(int argc, char* argv[]) {
    SubcommandResult result;
    if (argc < 2) {
        result.handled = true;
        result.showSubcommandHelp = true;
        return result;
    }

    const char* program = argv[0];
    const char* cmd = argv[1];

    if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        if (argc >= 3 && !isFlag(argv[2])) {
            result.handled = true;
            result.showSubcommandHelp = true;
            result.subcommandHelpTopic = argv[2];
            return result;
        }
        result.handled = true;
        result.showSubcommandHelp = true;
        return result;
    }

    if (strcmp(cmd, "dev") == 0) {
        result.useLegacy = true;
        result.rewrittenArgv.push_back(program);
        for (int i = 2; i < argc; ++i) {
            result.rewrittenArgv.push_back(argv[i]);
        }
        if (result.rewrittenArgv.size() == 1) {
            result.handled = true;
            printDevHelp(program);
            return result;
        }
        return result;
    }

    if (strcmp(cmd, "capabilities") == 0) {
        appendRewritten(&result.rewrittenArgv, program, {"--capabilities"}, argc, argv, 2);
        result.useLegacy = true;
        return result;
    }

    if (strcmp(cmd, "device") == 0) {
        if (argc < 3) {
            result.handled = true;
            result.showSubcommandHelp = true;
            result.subcommandHelpTopic = "device";
            return result;
        }
        const char* sub = argv[2];
        if (strcmp(sub, "list") == 0) {
            appendRewritten(&result.rewrittenArgv, program, {"--list"}, argc, argv, 3);
        } else if (strcmp(sub, "info") == 0) {
            appendRewritten(&result.rewrittenArgv, program, {"--list"}, argc, argv, 3);
        } else if (strcmp(sub, "plan") == 0) {
            appendRewritten(&result.rewrittenArgv, program, {"--device-plan"}, argc, argv, 3);
        } else if (strcmp(sub, "help") == 0 || strcmp(sub, "-h") == 0) {
            result.handled = true;
            result.showSubcommandHelp = true;
            result.subcommandHelpTopic = "device";
            return result;
        } else {
            result.handled = true;
            result.exitCode = 1;
            Logger::error("unknown device subcommand: " + std::string(sub));
            return result;
        }
        result.useLegacy = true;
        return result;
    }

    if (strcmp(cmd, "jailbreak") == 0) {
        std::vector<const char*> flags = {"--doctor-run", "--normal-ssh"};
        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "--execute") == 0) {
                flags.push_back("--jailbreak-execute");
                flags.push_back("--i-understand-jailbreak");
            } else if (strcmp(argv[i], "--external") == 0) {
                flags = {"--external-jailbreak", "--i-understand-jailbreak", "--normal-ssh"};
            }
        }
        appendRewritten(&result.rewrittenArgv, program, flags, argc, argv, 2);
        result.useLegacy = true;
        return result;
    }

    if (strcmp(cmd, "external-jailbreak") == 0) {
        std::vector<const char*> flags = {
            "--external-jailbreak", "--i-understand-jailbreak", "--normal-ssh"};
        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "--already-jailbroken") == 0) {
                flags.push_back("--already-jailbroken");
            } else if (strcmp(argv[i], "--store") == 0) {
                flags.push_back("--post-jb-store");
            } else if (strcmp(argv[i], "--store-install") == 0 && i + 1 < argc) {
                flags.push_back("--post-jb-store-install");
                flags.push_back(argv[++i]);
            }
        }
        appendRewritten(&result.rewrittenArgv, program, flags, argc, argv, 2);
        result.useLegacy = true;
        return result;
    }

    if (strcmp(cmd, "dfu-jailbreak") == 0) {
        appendRewritten(&result.rewrittenArgv, program,
                        {"--dfu-jailbreak", "--i-understand-jailbreak"}, argc, argv, 2);
        result.useLegacy = true;
        return result;
    }

    if (strcmp(cmd, "store") == 0) {
        if (argc < 3) {
            result.handled = true;
            result.showSubcommandHelp = true;
            result.subcommandHelpTopic = "store";
            return result;
        }
        const char* sub = argv[2];
        std::vector<const char*> flags;
        if (strcmp(sub, "init") == 0) {
            flags = {"--store-init"};
        } else if (strcmp(sub, "build") == 0) {
            flags = {"--store-build"};
        } else if (strcmp(sub, "add") == 0) {
            flags = {"--store-add"};
            if (argc < 4) {
                result.handled = true;
                result.exitCode = 1;
                Logger::error("store add requires DEB path");
                return result;
            }
            appendRewritten(&result.rewrittenArgv, program, flags, argc, argv, 3);
            result.useLegacy = true;
            return result;
        } else if (strcmp(sub, "sync") == 0) {
            flags = {"--store-sync", "--normal-ssh", "--store-sync-mode", "file"};
        } else if (strcmp(sub, "install") == 0) {
            flags = {"--store-install", "--normal-ssh"};
            if (argc < 4) {
                result.handled = true;
                result.exitCode = 1;
                Logger::error("store install requires PACKAGE name");
                return result;
            }
            appendRewritten(&result.rewrittenArgv, program, flags, argc, argv, 3);
            result.useLegacy = true;
            return result;
        } else if (strcmp(sub, "publish") == 0) {
            flags = {"--store-publish"};
            appendRewritten(&result.rewrittenArgv, program, flags, argc, argv, 3);
            result.useLegacy = true;
            return result;
        } else if (strcmp(sub, "help") == 0 || strcmp(sub, "-h") == 0) {
            result.handled = true;
            result.showSubcommandHelp = true;
            result.subcommandHelpTopic = "store";
            return result;
        } else {
            result.handled = true;
            result.exitCode = 1;
            Logger::error("unknown store subcommand: " + std::string(sub));
            return result;
        }
        appendRewritten(&result.rewrittenArgv, program, flags, argc, argv, 3);
        result.useLegacy = true;
        return result;
    }

    if (strcmp(cmd, "post-jb") == 0) {
        result.rewrittenArgv.push_back(program);
        result.rewrittenArgv.push_back("--post-jb-pipeline");
        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "--store") == 0) {
                result.rewrittenArgv.push_back("--post-jb-store");
            } else if (strcmp(argv[i], "--medicine") == 0) {
                result.rewrittenArgv.push_back("--medicine");
            } else if (strcmp(argv[i], "--normal-ssh") == 0) {
                result.rewrittenArgv.push_back("--normal-ssh");
            } else if (strcmp(argv[i], "--store-install") == 0 && i + 1 < argc) {
                result.rewrittenArgv.push_back("--post-jb-store-install");
                result.rewrittenArgv.push_back(argv[++i]);
            } else if (strcmp(argv[i], "--install-ipa") == 0 && i + 1 < argc) {
                result.rewrittenArgv.push_back("--install-ipa");
                result.rewrittenArgv.push_back(argv[++i]);
            } else {
                result.rewrittenArgv.push_back(argv[i]);
            }
        }
        result.useLegacy = true;
        return result;
    }

    if (strcmp(cmd, "analyze") == 0) {
        if (argc < 4) {
            result.handled = true;
            result.showSubcommandHelp = true;
            result.subcommandHelpTopic = "analyze";
            return result;
        }
        const char* kind = argv[2];
        const char* path = argv[3];
        std::vector<const char*> flags;
        if (strcmp(kind, "backup") == 0) {
            flags = {"--analyze-backup"};
        } else if (strcmp(kind, "crash") == 0) {
            flags = {"--analyze-crash"};
        } else if (strcmp(kind, "binary") == 0) {
            flags = {"--analyze-binary"};
        } else if (strcmp(kind, "dyldcache") == 0) {
            flags = {"--analyze-dyldcache"};
        } else {
            result.handled = true;
            result.exitCode = 1;
            Logger::error("unknown analyze kind: " + std::string(kind));
            return result;
        }
        result.rewrittenArgv.push_back(program);
        for (const char* f : flags) {
            result.rewrittenArgv.push_back(f);
        }
        result.rewrittenArgv.push_back(path);
        for (int i = 4; i < argc; ++i) {
            result.rewrittenArgv.push_back(argv[i]);
        }
        result.useLegacy = true;
        return result;
    }

    if (argv[1][0] != '-') {
        result.useLegacy = false;
        return result;
    }

    return result;
}

} /* namespace cli */
} /* namespace PP */
