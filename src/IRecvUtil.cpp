/*
 * IRecvUtil.cpp
 */

#include "IRecvUtil.h"
#include "Checkm8.h"

#include <unistd.h>
#include <vector>

namespace PP {
namespace irecv_util {

irecv_error_t openWithEcidRetry(irecv_client_t* client, uint64_t ecid) {
    irecv_error_t error = IRECV_E_UNKNOWN_ERROR;
    for (int attempt = 0; attempt < kOpenRetryCount; ++attempt) {
        error = irecv_open_with_ecid(client, ecid);
        if (error == IRECV_E_SUCCESS) {
            return error;
        }
        if (attempt + 1 < kOpenRetryCount) {
            sleep(static_cast<unsigned int>(kOpenRetryDelaySeconds));
        }
    }
    return error;
}

bool isRecoveryMode(int mode) {
    return mode == IRECV_K_RECOVERY_MODE_1 || mode == IRECV_K_RECOVERY_MODE_2;
}

bool isDfuMode(int mode) {
    return mode == IRECV_K_DFU_MODE || mode == IRECV_K_PORT_DFU_MODE;
}

uint64_t ecidFromClient(irecv_client_t client) {
    const struct irecv_device_info* info = irecv_get_device_info(client);
    if (info != nullptr && info->have_ecid) {
        return info->ecid;
    }
    return Checkm8::parseEcidFromSerial(serialFromClient(client));
}

std::string serialFromClient(irecv_client_t client) {
    const struct irecv_device_info* info = irecv_get_device_info(client);
    if (info != nullptr && info->serial_string != nullptr) {
        return std::string(info->serial_string);
    }
    return std::string();
}

std::string productTypeFromClient(irecv_client_t client) {
    irecv_device_t device = nullptr;
    if (irecv_devices_get_device_by_client(client, &device) == IRECV_E_SUCCESS &&
        device != nullptr && device->product_type != nullptr) {
        return std::string(device->product_type);
    }
    return std::string();
}

uint32_t cpidFromClient(irecv_client_t client) {
    const struct irecv_device_info* info = irecv_get_device_info(client);
    if (info != nullptr && info->have_cpid && info->cpid != 0) {
        return static_cast<uint32_t>(info->cpid);
    }
    return Checkm8::parseCpidFromSerial(serialFromClient(client));
}

void usbMemoryAddressFields(uint64_t address, uint16_t& w_value, uint16_t& w_index) {
    w_value = static_cast<uint16_t>(address & 0xFFFFu);
    w_index = static_cast<uint16_t>((address >> 16) & 0xFFFFu);
}

bool is64BitCpid(uint32_t cpid) {
    switch (cpid) {
        /* 32-bit ARM bootrom targets (A4–A6 class) */
        case 0x8940:
        case 0x8942:
        case 0x8945:
        case 0x8947:
        case 0x8950:
        case 0x8955:
        case 0x8960:
            return false;
        default:
            return true;
    }
}

int usbMemoryRead(irecv_client_t client, uint64_t address, unsigned char* data, uint16_t length) {
    uint16_t w_value = 0;
    uint16_t w_index = 0;
    usbMemoryAddressFields(address, w_value, w_index);
    return irecv_usb_control_transfer(client, 0xC0, 0, w_value, w_index, data, length, 1000);
}

int usbMemoryWrite(irecv_client_t client, uint64_t address, const unsigned char* data, uint16_t length) {
    uint16_t w_value = 0;
    uint16_t w_index = 0;
    usbMemoryAddressFields(address, w_value, w_index);
    return irecv_usb_control_transfer(
        client, 0x40, 0, w_value, w_index, const_cast<unsigned char*>(data), length, 1000);
}

uint64_t probeRecoveryEcid() {
    irecv_client_t client = nullptr;
    if (openWithEcidRetry(&client, 0) != IRECV_E_SUCCESS || client == nullptr) {
        return 0;
    }

    int mode = IRECV_K_RECOVERY_MODE_1;
    bool recovery = true;
    if (irecv_get_mode(client, &mode) == IRECV_E_SUCCESS) {
        recovery = isRecoveryMode(mode);
    }

    uint64_t ecid = 0;
    if (recovery) {
        ecid = ecidFromClient(client);
    }

    irecv_close(client);
    return ecid;
}

bool getCommandReturn(irecv_client_t client, uint32_t* returnCode) {
    if (client == nullptr || returnCode == nullptr) {
        return false;
    }
    const irecv_error_t err = irecv_getret(client, returnCode);
    return err == IRECV_E_SUCCESS;
}

IRecvCommandResult sendCommandWithResponse(irecv_client_t client,
                                           const std::string& command,
                                           const uint64_t recvLength) {
    IRecvCommandResult result;
    if (client == nullptr) {
        result.error = "null client";
        return result;
    }
    if (irecv_send_command(client, command.c_str()) != IRECV_E_SUCCESS) {
        result.error = "send_command failed";
        return result;
    }
    if (!getCommandReturn(client, &result.returnCode)) {
        result.error = "getret failed";
        return result;
    }
    if (recvLength > 0) {
        if (recvLength > 0xFFFFu) {
            result.error = "recv length exceeds USB limit";
            return result;
        }
        result.buffer.resize(recvLength);
        if (irecv_recv_buffer(client, reinterpret_cast<char*>(result.buffer.data()), recvLength) !=
            IRECV_E_SUCCESS) {
            result.error = "recv_buffer failed";
            result.buffer.clear();
            return result;
        }
    }
    result.success = true;
    return result;
}

} /* namespace irecv_util */
} /* namespace PP */
