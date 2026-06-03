/*
 * MobileDevice.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef MOBILEDEVICE_H_
#define MOBILEDEVICE_H_

#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <cstdlib>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/installation_proxy.h>
#include <plist/plist.h>

namespace PP {

namespace {

std::string plistStringValue(plist_t node) {
    if (node == nullptr) {
        return std::string();
    }
    char* value = nullptr;
    plist_get_string_val(node, &value);
    if (value == nullptr) {
        return std::string();
    }
    std::string result(value);
    free(value);
    return result;
}

const char* domainOrNull(const std::string& domain) {
    return domain.empty() ? nullptr : domain.c_str();
}

} /* anonymous namespace */

/**
 * @class MobileDevice
 * @brief Interface for communicating with iOS devices in normal mode
 */
class MobileDevice {
public:
    MobileDevice() {
        idevice_error_t error = idevice_new(&device_, NULL);
        if (error != IDEVICE_E_SUCCESS) {
            throw std::runtime_error("Failed to create new device handle");
        }
    }

    MobileDevice(const std::string& udid) {
        idevice_error_t error = idevice_new_with_options(&device_, udid.c_str(), IDEVICE_LOOKUP_USBMUX);
        if (error != IDEVICE_E_SUCCESS) {
            throw std::runtime_error("Failed to create new device handle with UDID");
        }
    }

    ~MobileDevice() {
        if (device_) {
            idevice_free(device_);
        }
    }

    std::string getDeviceName() const {
        return getValue("", "DeviceName");
    }

    std::string getDeviceType() const {
        return getValue("", "ProductType");
    }

    std::string getUDID() const {
        std::string udid;
        char* device_udid = nullptr;
        if (idevice_get_udid(device_, &device_udid) == IDEVICE_E_SUCCESS && device_udid != nullptr) {
            udid = device_udid;
            free(device_udid);
        }
        return udid;
    }

    std::string getValue(const std::string& domain, const std::string& key) const {
        std::string value;
        lockdownd_client_t client = nullptr;
        if (lockdownd_client_new_with_handshake(device_, &client, "MobileDevice") != LOCKDOWN_E_SUCCESS) {
            return value;
        }

        plist_t result = nullptr;
        if (lockdownd_get_value(client, domainOrNull(domain), key.c_str(), &result) == LOCKDOWN_E_SUCCESS) {
            value = plistStringValue(result);
        }
        if (result != nullptr) {
            plist_free(result);
        }
        lockdownd_client_free(client);
        return value;
    }

    bool setValue(const std::string& domain, const std::string& key, const std::string& value) const {
        lockdownd_error_t error = LOCKDOWN_E_UNKNOWN_ERROR;
        lockdownd_client_t client = nullptr;
        if (lockdownd_client_new_with_handshake(device_, &client, "MobileDevice") == LOCKDOWN_E_SUCCESS) {
            plist_t plistValue = plist_new_string(value.c_str());
            error = lockdownd_set_value(client, domainOrNull(domain), key.c_str(), plistValue);
            plist_free(plistValue);
            lockdownd_client_free(client);
        }
        return error == LOCKDOWN_E_SUCCESS;
    }

    std::vector<std::string> getInstalledApplications() const {
        std::vector<std::string> apps;
        lockdownd_client_t client = nullptr;
        if (lockdownd_client_new_with_handshake(device_, &client, "MobileDevice") != LOCKDOWN_E_SUCCESS) {
            return apps;
        }

        lockdownd_service_descriptor_t service = nullptr;
        lockdownd_error_t error = lockdownd_start_service(client, INSTPROXY_SERVICE_NAME, &service);
        if (error == LOCKDOWN_E_SUCCESS) {
            instproxy_client_t instproxy = nullptr;
            instproxy_error_t instproxy_error = instproxy_client_new(device_, service, &instproxy);
            if (instproxy_error == INSTPROXY_E_SUCCESS) {
                plist_t options = plist_new_dict();
                plist_dict_set_item(options, "ApplicationType", plist_new_string("User"));

                plist_t result = nullptr;
                instproxy_error = instproxy_browse(instproxy, options, &result);
                if (instproxy_error == INSTPROXY_E_SUCCESS && result != nullptr) {
                    const uint32_t count = plist_array_get_size(result);
                    for (uint32_t i = 0; i < count; i++) {
                        plist_t app = plist_array_get_item(result, i);
                        plist_t bundle_id_node = plist_dict_get_item(app, "CFBundleIdentifier");
                        if (bundle_id_node != nullptr) {
                            const std::string bundle_id = plistStringValue(bundle_id_node);
                            if (!bundle_id.empty()) {
                                apps.push_back(bundle_id);
                            }
                        }
                    }
                    plist_free(result);
                }
                plist_free(options);
                instproxy_client_free(instproxy);
            }
            lockdownd_service_descriptor_free(service);
        }
        lockdownd_client_free(client);
        return apps;
    }

private:
    idevice_t device_ = nullptr;
};

} /* namespace PP */

#endif /* MOBILEDEVICE_H_ */
