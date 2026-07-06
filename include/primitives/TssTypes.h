/*
 * TssTypes.h
 *
 * TSS / SHSH signing modes: stock idevicerestore vs futurerestore extensions.
 */

#ifndef PRIMITIVES_TSS_TYPES_H_
#define PRIMITIVES_TSS_TYPES_H_

#include <string>
#include <vector>

namespace PP {
namespace primitives {

/** How purplepois0n expects signing tickets to be obtained. */
enum class TssSigningMode {
    /** Live request to Apple TSS (idevicerestore / libtss / ipsw download tss). */
    StockLive,
    /** Saved APTicket (.shsh / .shsh2) — futurerestore unsigned restore path. */
    SavedApTicket,
    /** Auto: SavedApTicket when apticket path set, else StockLive. */
    Auto
};

/**
 * futurerestore-specific restore options (SEP/baseband ticket manifests).
 * See docs/book/deep/tss-futurerestore.md and tihmstar/futurerestore README.
 */
struct FutureRestoreOptions {
    std::string apticketPath;
    /** Additional -t tickets (Prometheus ApNonce collision speed-up). */
    std::vector<std::string> extraApticketPaths;
    bool latestSep = false;
    bool latestBaseband = false;
    bool noBaseband = false;
    bool updateInstall = false;
    /** -w: keep rebooting until ApNonce matches ticket (Prometheus collision). */
    bool waitApNonce = false;
    bool usePwndfu = false;
    bool justBoot = false;
    /** Optional boot-args after --just-boot (futurerestore tethered boot). */
    std::string justBootArgs;
    /** -e / --exit-recovery: exit recovery and quit without restoring. */
    bool exitRecovery = false;
    /** -d / --debug: verbose futurerestore logging. */
    bool debug = false;
    std::string sepPath;
    std::string sepManifestPath;
    std::string basebandPath;
    std::string basebandManifestPath;
    /** IPSW path to extract SEP BuildManifest (--latest-sep companion). */
    std::string sepManifestIpsw;
    /** IPSW path to extract baseband BuildManifest (--latest-baseband companion). */
    std::string bbManifestIpsw;
    /** Extra flags (PURPLEPOIS0N_FUTURERESTORE_ARGS), e.g. custom boot-args. */
    std::string extraArgs;
};

/** Stock live restore via idevicerestore (Apple still signs target IPSW). */
struct IdeviceRestoreOptions {
    bool updateInstall = false;
    bool debug = false;
};

const char* tssSigningModeToString(TssSigningMode mode);

/** Parse PURPLEPOIS0N_TSS_MODE (stock|futurerestore|auto). */
TssSigningMode tssSigningModeFromEnv();

/** Load FutureRestoreOptions from PURPLEPOIS0N_* env (see TssDelegate). */
FutureRestoreOptions futureRestoreOptionsFromEnv();

/** Load IdeviceRestoreOptions from PURPLEPOIS0N_* env. */
IdeviceRestoreOptions ideviceRestoreOptionsFromEnv();

} /* namespace primitives */
} /* namespace PP */

#endif /* PRIMITIVES_TSS_TYPES_H_ */
