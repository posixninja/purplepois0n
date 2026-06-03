/*
 * IRecvUtil.h
 *
 * Shared libirecovery open helpers with bounded retry (idevicerestore pattern).
 */

#ifndef IRECV_UTIL_H_
#define IRECV_UTIL_H_

#include <libirecovery.h>
#include <cstdint>
#include <string>

namespace PP {
namespace irecv_util {

constexpr int kOpenRetryCount = 10;
constexpr int kOpenRetryDelaySeconds = 1;

/** Open with irecv_open_with_ecid, retrying on failure. */
irecv_error_t openWithEcidRetry(irecv_client_t* client, uint64_t ecid);

/** True when mode is Recovery (1280/1281). */
bool isRecoveryMode(int mode);

/** True when mode is DFU (0x1227). */
bool isDfuMode(int mode);

/** Probe ECID from an open irecv client in Recovery mode; returns 0 if unavailable. */
uint64_t ecidFromClient(irecv_client_t client);

/** Serial string from irecv device info, or empty. */
std::string serialFromClient(irecv_client_t client);

/** Product type (e.g. iPhone12,1) from device database, or empty. */
std::string productTypeFromClient(irecv_client_t client);

/** CPID from device info or serial parse fallback. */
uint32_t cpidFromClient(irecv_client_t client);

/** Split a bootrom/DFU memory address into USB wValue/wIndex (32-bit layout). */
void usbMemoryAddressFields(uint64_t address, uint16_t& w_value, uint16_t& w_index);

/** True when CPID maps to a 64-bit ARM SoC (A7+). */
bool is64BitCpid(uint32_t cpid);

/** Read device memory via irecv control transfer (32-bit address encoding). */
int usbMemoryRead(irecv_client_t client, uint64_t address, unsigned char* data, uint16_t length);

/** Write device memory via irecv control transfer (32-bit address encoding). */
int usbMemoryWrite(irecv_client_t client, uint64_t address, const unsigned char* data, uint16_t length);

/** Probe ECID by opening the first Recovery device; returns 0 if unavailable. */
uint64_t probeRecoveryEcid();

} /* namespace irecv_util */
} /* namespace PP */

#endif /* IRECV_UTIL_H_ */
