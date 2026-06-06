/*
 * TssTypes.cpp
 */

#include "primitives/TssTypes.h"

#include <cstdlib>
#include <cstring>

namespace PP {
namespace primitives {

const char* tssSigningModeToString(TssSigningMode mode) {
    switch (mode) {
        case TssSigningMode::StockLive:
            return "stock-live";
        case TssSigningMode::SavedApTicket:
            return "saved-apticket";
        case TssSigningMode::Auto:
            return "auto";
    }
    return "unknown";
}

TssSigningMode tssSigningModeFromEnv() {
    const char* mode = std::getenv("PURPLEPOIS0N_TSS_MODE");
    if (mode == nullptr || mode[0] == '\0') {
        return TssSigningMode::Auto;
    }
    if (strcmp(mode, "stock") == 0 || strcmp(mode, "live") == 0) {
        return TssSigningMode::StockLive;
    }
    if (strcmp(mode, "futurerestore") == 0 || strcmp(mode, "saved") == 0) {
        return TssSigningMode::SavedApTicket;
    }
    return TssSigningMode::Auto;
}

namespace {

bool envTruthy(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0' && strcmp(v, "0") != 0;
}

std::string envString(const char* name) {
    const char* v = std::getenv(name);
    return (v != nullptr) ? std::string(v) : std::string();
}

} /* anonymous */

FutureRestoreOptions futureRestoreOptionsFromEnv() {
    FutureRestoreOptions opts;
    opts.apticketPath = envString("PURPLEPOIS0N_APTICKET");
    opts.latestSep = envTruthy("PURPLEPOIS0N_FUTURERESTORE_LATEST_SEP");
    opts.latestBaseband = envTruthy("PURPLEPOIS0N_FUTURERESTORE_LATEST_BASEBAND");
    opts.noBaseband = envTruthy("PURPLEPOIS0N_FUTURERESTORE_NO_BASEBAND");
    opts.updateInstall = envTruthy("PURPLEPOIS0N_FUTURERESTORE_UPDATE");
    opts.waitApNonce = envTruthy("PURPLEPOIS0N_FUTURERESTORE_WAIT_NONCE");
    opts.usePwndfu = envTruthy("PURPLEPOIS0N_FUTURERESTORE_USE_PWNDFU");
    opts.justBoot = envTruthy("PURPLEPOIS0N_FUTURERESTORE_JUST_BOOT");
    opts.sepPath = envString("PURPLEPOIS0N_FUTURERESTORE_SEP");
    opts.sepManifestPath = envString("PURPLEPOIS0N_FUTURERESTORE_SEP_MANIFEST");
    opts.basebandPath = envString("PURPLEPOIS0N_FUTURERESTORE_BASEBAND");
    opts.basebandManifestPath = envString("PURPLEPOIS0N_FUTURERESTORE_BASEBAND_MANIFEST");
    opts.sepManifestIpsw = envString("PURPLEPOIS0N_SEP_IPSW");
    opts.bbManifestIpsw = envString("PURPLEPOIS0N_BB_IPSW");
    if (opts.sepManifestIpsw.empty()) {
        opts.sepManifestIpsw = envString("PURPLEPOIS0N_LATEST_IPSW");
    }
    if (opts.bbManifestIpsw.empty()) {
        opts.bbManifestIpsw = envString("PURPLEPOIS0N_LATEST_IPSW");
    }
    opts.extraArgs = envString("PURPLEPOIS0N_FUTURERESTORE_ARGS");

    const char* flags = std::getenv("PURPLEPOIS0N_FUTURERESTORE_FLAGS");
    if (flags != nullptr && flags[0] != '\0') {
        if (strstr(flags, "--latest-sep") != nullptr) {
            opts.latestSep = true;
        }
        if (strstr(flags, "--latest-baseband") != nullptr) {
            opts.latestBaseband = true;
        }
        if (strstr(flags, "--no-baseband") != nullptr) {
            opts.noBaseband = true;
        }
        if (strstr(flags, "--update") != nullptr || strstr(flags, "-u") != nullptr) {
            opts.updateInstall = true;
        }
        if (strstr(flags, "--wait") != nullptr || strstr(flags, "-w") != nullptr) {
            opts.waitApNonce = true;
        }
        if (strstr(flags, "--use-pwndfu") != nullptr) {
            opts.usePwndfu = true;
        }
        if (strstr(flags, "--just-boot") != nullptr) {
            opts.justBoot = true;
        }
        if (opts.extraArgs.empty()) {
            opts.extraArgs = flags;
        }
    }
    return opts;
}

} /* namespace primitives */
} /* namespace PP */
