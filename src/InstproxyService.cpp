/*
 * InstproxyService.cpp
 */

#include "InstproxyService.h"
#include "Logger.h"

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/installation_proxy.h>
#include <plist/plist.h>

#include <cstring>
#include <unistd.h>

namespace PP {

namespace {

struct InstallState {
    bool finished = false;
    bool success = false;
    std::string lastStatus;
    int percent = -1;
};

void installStatusCallback(plist_t /*command*/, plist_t status, void* userData) {
    InstallState* state = static_cast<InstallState*>(userData);
    if (status == nullptr || state == nullptr) {
        return;
    }

    plist_t node = plist_dict_get_item(status, "Status");
    if (node != nullptr) {
        char* text = nullptr;
        plist_get_string_val(node, &text);
        if (text != nullptr) {
            state->lastStatus = text;
            free(text);
        }
    }

    node = plist_dict_get_item(status, "PercentComplete");
    if (node != nullptr) {
        uint64_t value = 0;
        plist_get_uint_val(node, &value);
        state->percent = static_cast<int>(value);
        if (state->percent >= 0) {
            Logger::info("  [Sideload] install progress: " + std::to_string(state->percent) + "%");
        }
    }

    if (state->lastStatus == "Complete") {
        state->finished = true;
        state->success = true;
    } else if (state->lastStatus.find("Error") != std::string::npos ||
               state->lastStatus == "APIInternalError") {
        state->finished = true;
        state->success = false;
    } else if (state->lastStatus == "Processing" || state->lastStatus == "CreatingStagingDirectory" ||
               state->lastStatus == "Installing") {
        Logger::info("  [Sideload] " + state->lastStatus);
    }
}

} /* anonymous */

InstproxyService::InstproxyService(const std::string& udid) : mUdid(udid) {}

bool InstproxyService::probe(std::string* errorMessage) {
    if (mUdid.empty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "missing UDID";
        }
        return false;
    }

    idevice_t device = nullptr;
    if (idevice_new_with_options(&device, mUdid.c_str(), IDEVICE_LOOKUP_USBMUX) != IDEVICE_E_SUCCESS) {
        if (errorMessage != nullptr) {
            *errorMessage = "idevice connect failed";
        }
        return false;
    }

    instproxy_client_t client = nullptr;
    const instproxy_error_t err =
        instproxy_client_start_service(device, &client, "purplepois0n");
    if (err != INSTPROXY_E_SUCCESS || client == nullptr) {
        idevice_free(device);
        if (errorMessage != nullptr) {
            *errorMessage = "instproxy_client_start_service failed";
        }
        return false;
    }

    instproxy_client_free(client);
    idevice_free(device);
    return true;
}

bool InstproxyService::install(const std::string& ipaPath, std::string* errorMessage) {
    if (mUdid.empty() || ipaPath.empty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "missing UDID or IPA path";
        }
        return false;
    }

    idevice_t device = nullptr;
    if (idevice_new_with_options(&device, mUdid.c_str(), IDEVICE_LOOKUP_USBMUX) != IDEVICE_E_SUCCESS) {
        if (errorMessage != nullptr) {
            *errorMessage = "idevice connect failed";
        }
        return false;
    }

    instproxy_client_t client = nullptr;
    instproxy_error_t err =
        instproxy_client_start_service(device, &client, "purplepois0n");
    if (err != INSTPROXY_E_SUCCESS || client == nullptr) {
        idevice_free(device);
        if (errorMessage != nullptr) {
            *errorMessage = "instproxy_client_start_service failed";
        }
        return false;
    }

    InstallState state;
    plist_t options = plist_new_dict();
    err = instproxy_install(client, ipaPath.c_str(), options, installStatusCallback, &state);
    plist_free(options);

    if (err != INSTPROXY_E_SUCCESS) {
        instproxy_client_free(client);
        idevice_free(device);
        if (errorMessage != nullptr) {
            *errorMessage = "instproxy_install returned error " + std::to_string(err);
        }
        return false;
    }

    for (int i = 0; i < 600 && !state.finished; ++i) {
        usleep(100000);
    }

    instproxy_client_free(client);
    idevice_free(device);

    if (!state.finished) {
        if (errorMessage != nullptr) {
            *errorMessage = "install timed out (last: " + state.lastStatus + ")";
        }
        return false;
    }

    if (!state.success) {
        if (errorMessage != nullptr) {
            *errorMessage = "install failed: " + state.lastStatus;
        }
        return false;
    }

    Logger::info("  [Sideload] install complete");
    return true;
}

} /* namespace PP */
