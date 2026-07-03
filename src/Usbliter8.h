/*
 * Usbliter8.h
 *
 * usbliter8 (bootrom DMA-underflow exploit for A12/A13/S4/S5) support: SoC
 * identification, support checks, and post-exploit control via the external
 * usbliter8ctl tool when present.
 *
 * Unlike checkm8, the exploit's DMA-underflow delivery cannot be driven from
 * a normal Mac/PC USB host stack — it requires an RP2350 microcontroller
 * bridge running the usbliter8 firmware (see
 * https://github.com/prdgmshift/usbliter8). purplepois0n can only detect the
 * post-exploit PWND state and drive usbliter8ctl for follow-on actions (boot
 * raw iBoot / demote production mode). Educational/research use.
 */

#ifndef USBLITER8_H_
#define USBLITER8_H_

#include <cstdint>
#include <string>

namespace PP {

class DFUDevice;
class DeviceManager;

/**
 * @brief Result of a usbliter8 state check / post-exploit control attempt
 */
enum class Usbliter8Result {
    Success,
    UnsupportedDevice,
    AlreadyPwned,
    RequiresHardwareBridge,
    ExternalToolMissing,
    ExternalToolFailed,
    DeviceOpenFailed,
    UnknownError
};

/**
 * @brief Post-exploit action to request from usbliter8ctl once a device
 *        already shows the usbliter8 PWND marker.
 */
enum class Usbliter8Action {
    Boot,   /* boot raw iBoot */
    Demote  /* demote production mode */
};

/**
 * @brief Identifies a DFU device relevant to usbliter8
 */
struct Usbliter8DeviceInfo {
    uint32_t cpid = 0;
    uint64_t ecid = 0;
    std::string serial;
    std::string deviceType;
    std::string socName;
    bool supported = false;
    bool pwned = false;
};

/**
 * @class Usbliter8
 * @brief CPID classification and usbliter8 state/control (external
 *        usbliter8ctl or documented hardware-bridge instructions)
 */
class Usbliter8 {
public:
    static Usbliter8DeviceInfo probe(DFUDevice& device);

    static bool isSupportedCpid(uint32_t cpid);
    static std::string resultToString(Usbliter8Result result);
    static bool serialIndicatesPwned(const std::string& serial);

    /**
     * @brief Check pwned state / drive post-exploit control using info
     *        gathered while DFU was open.
     * @note The DFU USB handle must be closed before calling this (exclusive access).
     * @param info Output from probe()
     * @param action Post-exploit action to request from usbliter8ctl when already pwned.
     */
    static Usbliter8Result runExploit(const Usbliter8DeviceInfo& info,
                                       Usbliter8Action action = Usbliter8Action::Boot);

    /** @brief Probe DFU device, release USB, check/drive usbliter8 state (see runExploit). */
    static bool runUsbliter8(DeviceManager& manager);
};

} /* namespace PP */

#endif /* USBLITER8_H_ */
