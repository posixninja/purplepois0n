/*
 * Checkm8.h
 *
 * Bootrom (checkm8) exploit orchestration for DFU-mode devices (A5–A11).
 * Educational/research use — integrates with DFUDevice and optional external tools.
 */

#ifndef CHECKM8_H_
#define CHECKM8_H_

#include <cstdint>
#include <string>

namespace PP {

class DFUDevice;
class DeviceManager;

/**
 * @brief Result of a checkm8 exploit attempt
 */
enum class Checkm8Result {
    Success,
    UnsupportedDevice,
    AlreadyPwned,
    ExternalToolMissing,
    ExternalToolFailed,
    DeviceOpenFailed,
    UnknownError
};

/**
 * @brief Identifies a DFU device relevant to checkm8
 */
struct Checkm8DeviceInfo {
    uint32_t cpid = 0;
    uint64_t ecid = 0;
    std::string serial;
    std::string deviceType;
    std::string socName;
    bool supported = false;
    bool pwned = false;
};

/**
 * @class Checkm8
 * @brief CPID classification and checkm8 trigger (external gaster/ipwndfu or documented fallback)
 */
class Checkm8 {
public:
    static Checkm8DeviceInfo probe(DFUDevice& device);

    static bool isSupportedCpid(uint32_t cpid);
    static bool isKnownUnsupportedCpid(uint32_t cpid);
    static std::string cpidToSocName(uint32_t cpid);
    static std::string resultToString(Checkm8Result result);

    /**
     * @brief Run checkm8 using device info gathered while DFU was open.
     * @note The DFU USB handle must be closed before calling this (exclusive access).
     * @param info Output from probe()
     * @param preferExternal If true, try gaster/ipwndfu (required for exploit delivery today)
     */
    static Checkm8Result runExploit(const Checkm8DeviceInfo& info, bool preferExternal = true);

    static uint32_t parseCpidFromSerial(const std::string& serial);
    static uint64_t parseEcidFromSerial(const std::string& serial);
    static bool serialIndicatesPwned(const std::string& serial);

    /** @brief Probe DFU device, release USB, run exploit (see runExploit). */
    static bool runCheckm8(DeviceManager& manager);
};

} /* namespace PP */

#endif /* CHECKM8_H_ */
