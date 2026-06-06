/*
 * TssClient.cpp
 */

#include "TssClient.h"
#include "ToolRunner.h"
#include "Logger.h"
#include "primitives/ITransport.h"
#include "primitives/TssTypes.h"

#include <plist/plist.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef PURPLEPOIS0N_HAVE_LIBTATSU
#include <libtatsu/tss.h>
#endif

namespace PP {

namespace {

bool writeBytesToFile(const std::string& path, const unsigned char* data, unsigned int length) {
    FILE* fp = fopen(path.c_str(), "wb");
    if (fp == nullptr) {
        return false;
    }
    const size_t written = fwrite(data, 1, length, fp);
    fclose(fp);
    return written == length;
}

std::string resolveSepManifestSource(const primitives::FutureRestoreOptions& fr) {
    if (!fr.sepManifestPath.empty()) {
        return fr.sepManifestPath;
    }
    if (fr.latestSep && !fr.sepManifestIpsw.empty()) {
        return fr.sepManifestIpsw;
    }
    return std::string();
}

std::string resolveBbManifestSource(const primitives::FutureRestoreOptions& fr) {
    if (!fr.basebandManifestPath.empty()) {
        return fr.basebandManifestPath;
    }
    if (fr.latestBaseband && !fr.bbManifestIpsw.empty()) {
        return fr.bbManifestIpsw;
    }
    return std::string();
}

#ifdef PURPLEPOIS0N_HAVE_LIBTATSU

bool writePlistToFile(plist_t node, const std::string& path) {
    char* xml = nullptr;
    uint32_t length = 0;
    plist_to_xml(node, &xml, &length);
    if (xml == nullptr) {
        return false;
    }
    std::ofstream out(path.c_str(), std::ios::binary);
    out.write(xml, length);
    const bool ok = out.good();
    free(xml);
    return ok;
}

bool pathEndsWithIpsw(const std::string& path) {
    return path.size() >= 5 && path.compare(path.size() - 5, 5, ".ipsw") == 0;
}

std::string extractBuildManifestFromIpsw(const std::string& ipswPath) {
    const std::string outPath = "/tmp/pp-BuildManifest.plist";
    std::vector<std::string> argv;
    argv.push_back("/bin/sh");
    argv.push_back("-c");
    argv.push_back("unzip -p '" + ipswPath + "' BuildManifest.plist > '" + outPath + "'");
    const CommandResult result = ToolRunner::run(argv);
    if (result.exitCode != 0) {
        return std::string();
    }
    return outPath;
}

plist_t loadBuildManifestRoot(const std::string& pathOrIpsw, std::string* resolvedPath) {
    if (pathOrIpsw.empty()) {
        return nullptr;
    }
    std::string manifestPath = pathOrIpsw;
    if (pathEndsWithIpsw(pathOrIpsw)) {
        manifestPath = extractBuildManifestFromIpsw(pathOrIpsw);
        if (manifestPath.empty()) {
            return nullptr;
        }
    }
    plist_t manifest = nullptr;
    if (plist_read_from_file(manifestPath.c_str(), &manifest, nullptr) != PLIST_ERR_SUCCESS ||
        manifest == nullptr) {
        return nullptr;
    }
    if (resolvedPath != nullptr) {
        *resolvedPath = manifestPath;
    }
    return manifest;
}

plist_t findBuildIdentity(plist_t manifest, uint32_t cpid, uint32_t boardId) {
    plist_t identities = plist_dict_get_item(manifest, "BuildIdentities");
    if (identities == nullptr || plist_get_node_type(identities) != PLIST_ARRAY) {
        return nullptr;
    }
    const uint32_t count = plist_array_get_size(identities);
    plist_t fallback = nullptr;
    for (uint32_t i = 0; i < count; ++i) {
        plist_t identity = plist_array_get_item(identities, i);
        if (identity == nullptr) {
            continue;
        }
        plist_t info = plist_dict_get_item(identity, "Info");
        if (info == nullptr) {
            continue;
        }
        plist_t chipNode = plist_dict_get_item(info, "ApChipID");
        plist_t boardNode = plist_dict_get_item(info, "ApBoardID");
        uint64_t chip = 0;
        uint64_t board = 0;
        if (chipNode != nullptr) {
            plist_get_uint_val(chipNode, &chip);
        }
        if (boardNode != nullptr) {
            plist_get_uint_val(boardNode, &board);
        }
        if (cpid != 0 && static_cast<uint32_t>(chip) == cpid) {
            if (boardId == 0 || static_cast<uint32_t>(board) == boardId) {
                return identity;
            }
            if (fallback == nullptr) {
                fallback = identity;
            }
        }
    }
    if (fallback != nullptr) {
        return fallback;
    }
    return count > 0 ? plist_array_get_item(identities, 0) : nullptr;
}

plist_t buildParametersFromIdentity(plist_t identity, bool includeManifest) {
    plist_t parameters = plist_new_dict();
    if (tss_parameters_add_from_manifest(parameters, identity, includeManifest) != 0) {
        plist_free(parameters);
        return nullptr;
    }
    return parameters;
}

void applyLiveDeviceFields(plist_t parameters,
                           const TssLiveDeviceParams& params) {
    if (params.ecid != 0) {
        plist_dict_set_item(parameters, "ApECID", plist_new_uint(params.ecid));
    }
    if (!params.apNonce.empty()) {
        plist_dict_set_item(parameters, "ApNonce",
                            plist_new_data(reinterpret_cast<const char*>(params.apNonce.data()),
                                           params.apNonce.size()));
    }
    plist_dict_set_item(parameters, "ApProductionMode", plist_new_bool(1));
}

/** BuildIdentity parameters from manifest (IPSW or BuildManifest.plist). */
plist_t buildOverridesFromManifestPath(const std::string& manifestPathOrIpsw,
                                       uint32_t cpid,
                                       uint32_t boardId) {
    std::string resolved;
    plist_t manifest = loadBuildManifestRoot(manifestPathOrIpsw, &resolved);
    if (manifest == nullptr) {
        return nullptr;
    }
    plist_t identity = findBuildIdentity(manifest, cpid, boardId);
    plist_t overrides = nullptr;
    if (identity != nullptr) {
        overrides = buildParametersFromIdentity(identity, false);
    }
    plist_free(manifest);
    return overrides;
}

bool addSepTags(plist_t request,
                plist_t parameters,
                const primitives::FutureRestoreOptions& fr,
                uint32_t cpid,
                uint32_t boardId,
                bool stockLive) {
    const std::string sepSource = resolveSepManifestSource(fr);
    if (!sepSource.empty()) {
        plist_t sepOverrides = buildOverridesFromManifestPath(sepSource, cpid, boardId);
        if (sepOverrides == nullptr) {
            Logger::warn("  [TSS]    libtatsu: failed SEP manifest from " + sepSource);
            return false;
        }
        const int rc = tss_request_add_se_tags(request, parameters, sepOverrides);
        plist_free(sepOverrides);
        if (rc != 0) {
            Logger::warn("  [TSS]    libtatsu: tss_request_add_se_tags failed");
            return false;
        }
        Logger::info("  [TSS]    libtatsu: SEP tags from " + sepSource);
        return true;
    }
    if (fr.latestSep) {
        Logger::warn("  [TSS]    --latest-sep needs --sep-ipsw or PURPLEPOIS0N_SEP_IPSW");
        return false;
    }
    if (stockLive) {
        const int rc = tss_request_add_se_tags(request, parameters, nullptr);
        if (rc != 0) {
            Logger::info("  [TSS]    libtatsu: no SEP tags in target manifest (Wi‑Fi-only?)");
            return true;
        }
        Logger::info("  [TSS]    libtatsu: SEP tags from target IPSW");
        return true;
    }
    return true;
}

bool addBasebandTags(plist_t request,
                     plist_t parameters,
                     const primitives::FutureRestoreOptions& fr,
                     uint32_t cpid,
                     uint32_t boardId,
                     bool stockLive) {
    if (fr.noBaseband) {
        Logger::info("  [TSS]    libtatsu: skipping baseband (--no-baseband)");
        return true;
    }

    const std::string bbSource = resolveBbManifestSource(fr);
    if (!bbSource.empty()) {
        plist_t bbOverrides = buildOverridesFromManifestPath(bbSource, cpid, boardId);
        if (bbOverrides == nullptr) {
            Logger::warn("  [TSS]    libtatsu: failed baseband manifest from " + bbSource);
            return false;
        }
        const int rc = tss_request_add_baseband_tags(request, parameters, bbOverrides);
        plist_free(bbOverrides);
        if (rc != 0) {
            Logger::warn("  [TSS]    libtatsu: tss_request_add_baseband_tags failed");
            return false;
        }
        Logger::info("  [TSS]    libtatsu: baseband tags from " + bbSource);
        return true;
    }
    if (fr.latestBaseband) {
        Logger::warn("  [TSS]    --latest-baseband needs --bb-ipsw or PURPLEPOIS0N_BB_IPSW");
        return false;
    }
    if (stockLive) {
        const int rc = tss_request_add_baseband_tags(request, parameters, nullptr);
        if (rc != 0) {
            Logger::info("  [TSS]    libtatsu: no baseband tags in target manifest");
            return true;
        }
        Logger::info("  [TSS]    libtatsu: baseband tags from target IPSW");
        return true;
    }
    return true;
}

bool requestViaLibtatsu(const primitives::ExecutionContext& context,
                        const TssLiveDeviceParams& params,
                        const std::string& shshPath) {
    const std::string ipswPath = context.ipswPath;
    if (ipswPath.empty()) {
        return false;
    }

    std::string resolved;
    plist_t manifest = loadBuildManifestRoot(ipswPath, &resolved);
    if (manifest == nullptr) {
        Logger::error("  [TSS]    failed to load BuildManifest from " + ipswPath);
        return false;
    }

    plist_t identity = findBuildIdentity(manifest, params.cpid, params.boardId);
    if (identity == nullptr) {
        Logger::error("  [TSS]    no matching BuildIdentity in manifest");
        plist_free(manifest);
        return false;
    }

    plist_t parameters = buildParametersFromIdentity(identity, true);
    if (parameters == nullptr) {
        Logger::error("  [TSS]    tss_parameters_add_from_manifest failed");
        plist_free(manifest);
        return false;
    }
    applyLiveDeviceFields(parameters, params);

    const primitives::FutureRestoreOptions fr = context.futureRestore;
    const bool stockLive = fr.apticketPath.empty() && !fr.latestSep && !fr.latestBaseband &&
                           fr.sepManifestPath.empty() && fr.basebandManifestPath.empty();

    plist_t request = tss_request_new(nullptr);
    if (request == nullptr) {
        plist_free(parameters);
        plist_free(manifest);
        return false;
    }

    tss_request_add_common_tags(request, parameters, nullptr);
    tss_request_add_ap_tags(request, parameters, nullptr);
    tss_request_add_ap_recovery_tags(request, parameters, nullptr);
    tss_request_add_ap_img4_tags(request, parameters);

    addSepTags(request, parameters, fr, params.cpid, params.boardId, stockLive);
    addBasebandTags(request, parameters, fr, params.cpid, params.boardId, stockLive);

    plist_t response = tss_request_send(request, nullptr);
    plist_free(request);
    plist_free(parameters);
    plist_free(manifest);

    if (response == nullptr) {
        Logger::error("  [TSS]    libtatsu: no response from Apple TSS");
        return false;
    }

    const bool saved = writePlistToFile(response, shshPath);
    plist_free(response);
    if (saved) {
        Logger::info("  [TSS]    libtatsu: saved live SHSH → " + shshPath);
    }
    return saved;
}

#endif /* PURPLEPOIS0N_HAVE_LIBTATSU */

plist_t findApImg4TicketNode(plist_t root) {
    if (root == nullptr) {
        return nullptr;
    }
    plist_t ticket = plist_dict_get_item(root, "ApImg4Ticket");
    if (ticket != nullptr && plist_get_node_type(ticket) == PLIST_DATA) {
        return ticket;
    }
    /* Some blobs nest under RestoreApImg4Ticket or similar */
    const char* keys[] = {"RestoreApImg4Ticket", "ApTicket", nullptr};
    for (int i = 0; keys[i] != nullptr; ++i) {
        ticket = plist_dict_get_item(root, keys[i]);
        if (ticket != nullptr && plist_get_node_type(ticket) == PLIST_DATA) {
            return ticket;
        }
    }
    return nullptr;
}

} /* anonymous namespace */

bool TssClient::isLibtatsuLinked() {
#ifdef PURPLEPOIS0N_HAVE_LIBTATSU
    return true;
#else
    return false;
#endif
}

TssLiveDeviceParams TssClient::paramsFromContext(const primitives::ExecutionContext& context) {
    TssLiveDeviceParams params;
    params.ecid = context.ecid;
    params.cpid = context.cpid;
    params.productType = context.productType;
    params.iosVersion = context.iosVersion;

    if (context.transport != nullptr) {
        params.apNonce = context.transport->getApNonceBytes();
        if (params.apNonce.empty()) {
            const std::string nonceHex = context.transport->getDeviceEnv("NONCE");
            if (!nonceHex.empty() && nonceHex.size() >= 2) {
                for (size_t i = 0; i + 1 < nonceHex.size(); i += 2) {
                    const char hex[3] = {nonceHex[i], nonceHex[i + 1], '\0'};
                    params.apNonce.push_back(static_cast<uint8_t>(strtoul(hex, nullptr, 16)));
                }
            }
        }
    }

    params.boardId = context.boardId;
    return params;
}

bool TssClient::saveShshViaIpsw(const primitives::ExecutionContext& context,
                                const std::string& outputPath) {
    const std::string ipswBin = ToolRunner::findIpswExecutable();
    if (ipswBin.empty() || outputPath.empty()) {
        return false;
    }

    std::vector<std::string> argv;
    argv.push_back(ipswBin);
    argv.push_back("download");
    argv.push_back("tss");
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
    argv.push_back("--output");
    argv.push_back(outputPath);

    Logger::info("  [TSS]    fetching SHSH via ipsw download tss...");
    const CommandResult result = ToolRunner::run(argv);
    if (result.exitCode == 0) {
        Logger::info("  [TSS]    saved SHSH → " + outputPath);
        return true;
    }
    Logger::warn("  [TSS]    ipsw download tss failed (exit " + std::to_string(result.exitCode) + ")");
    return false;
}

bool TssClient::extractIm4mFromShshPlist(const std::string& shshPath,
                                          const std::string& im4mPath) {
    plist_t root = nullptr;
    if (plist_read_from_file(shshPath.c_str(), &root, nullptr) != PLIST_ERR_SUCCESS ||
        root == nullptr) {
        return false;
    }

    plist_t ticketNode = findApImg4TicketNode(root);
    if (ticketNode == nullptr) {
        plist_free(root);
        return false;
    }

    char* data = nullptr;
    uint64_t length = 0;
    plist_get_data_val(ticketNode, &data, &length);
    plist_free(root);
    if (data == nullptr || length == 0) {
        return false;
    }

    const bool ok = writeBytesToFile(im4mPath, reinterpret_cast<unsigned char*>(data),
                                      static_cast<unsigned int>(length));
    free(data);
    if (ok) {
        Logger::info("  [TSS]    extracted IM4M (libplist) → " + im4mPath);
    }
    return ok;
}

bool TssClient::fetchLiveShsh(const primitives::ExecutionContext& context,
                              const std::string& shshPath) {
    if (shshPath.empty()) {
        return false;
    }

    const TssLiveDeviceParams params = paramsFromContext(context);

#ifdef PURPLEPOIS0N_HAVE_LIBTATSU
    if (!context.ipswPath.empty() && params.ecid != 0) {
        Logger::info("  [TSS]    live request via libtatsu (idevicerestore tss.c lineage)...");
        if (requestViaLibtatsu(context, params, shshPath)) {
            return true;
        }
        Logger::warn("  [TSS]    libtatsu failed — falling back to ipsw download tss");
    }
#else
    if (!context.ipswPath.empty()) {
        Logger::info("  [TSS]    libtatsu not linked — build with make LIBTATSU=1");
    }
#endif

    return saveShshViaIpsw(context, shshPath);
}

void TssClient::logFuturerestoreSepBbPlan(const primitives::ExecutionContext& context) {
    const primitives::FutureRestoreOptions fr = context.futureRestore;

    if (fr.noBaseband) {
        Logger::info("  [TSS]    libtatsu plan: skip baseband (--no-baseband)");
    } else if (fr.latestBaseband) {
        const std::string bb = resolveBbManifestSource(fr);
        if (!bb.empty()) {
            Logger::info("  [TSS]    libtatsu plan: latest baseband manifest → " + bb);
        } else {
            Logger::warn("  [TSS]    libtatsu plan: --latest-baseband needs --bb-ipsw");
        }
    } else if (!fr.basebandManifestPath.empty()) {
        Logger::info("  [TSS]    libtatsu plan: baseband manifest → " + fr.basebandManifestPath);
    } else if (!context.ipswPath.empty()) {
        Logger::info("  [TSS]    libtatsu plan: baseband tags from target IPSW");
    }

    if (fr.latestSep) {
        const std::string sep = resolveSepManifestSource(fr);
        if (!sep.empty()) {
            Logger::info("  [TSS]    libtatsu plan: latest SEP manifest → " + sep);
        } else {
            Logger::warn("  [TSS]    libtatsu plan: --latest-sep needs --sep-ipsw");
        }
    } else if (!fr.sepManifestPath.empty()) {
        Logger::info("  [TSS]    libtatsu plan: SEP manifest → " + fr.sepManifestPath);
    } else if (!context.ipswPath.empty()) {
        Logger::info("  [TSS]    libtatsu plan: SEP tags from target IPSW");
    }
}

bool TssClient::fetchLiveIm4m(const primitives::ExecutionContext& context,
                              const std::string& shshPath,
                              const std::string& im4mPath) {
    if (!fetchLiveShsh(context, shshPath)) {
        return false;
    }
    if (extractIm4mFromShshPlist(shshPath, im4mPath)) {
        return true;
    }
    /* ipsw img4 im4m extract fallback */
    const std::string ipswBin = ToolRunner::findIpswExecutable();
    if (ipswBin.empty()) {
        return false;
    }
    std::vector<std::string> argv;
    argv.push_back(ipswBin);
    argv.push_back("img4");
    argv.push_back("im4m");
    argv.push_back("extract");
    argv.push_back("--output");
    argv.push_back(im4mPath);
    argv.push_back(shshPath);
    return ToolRunner::run(argv).exitCode == 0;
}

} /* namespace PP */
