/*
 * Syringe.h
 *
 * Unified USB/bootrom communication abstraction.
 * Provides device communication layer for all bootrom exploits (checkm8, usbliter8, etc.)
 * and post-exploit payloads (cyanide iBoot, anthrax ramdisk).
 * Handles DFU, recovery mode, and normal mode transports uniformly.
 */

#ifndef SYRINGE_H_
#define SYRINGE_H_

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace PP {

class DeviceManager;

enum class SyringeTransport {
    DFU,
    Recovery,
    Normal
};

enum class SyringeCommand {
    GetCpid,
    GetEcid,
    GetBoardConfig,
    ReadMemory,
    WriteMemory,
    ExecuteCode,
    SendFile,
    ReceiveFile,
    Exploit,
    Custom
};

/**
 * @struct SyringeRequest
 * @brief Low-level device communication request.
 */
struct SyringeRequest {
    SyringeCommand cmd;
    std::vector<uint8_t> data;
    uint64_t addr;
    uint64_t size;
    std::string tag;
};

/**
 * @struct SyringeResponse
 * @brief Low-level device communication response.
 */
struct SyringeResponse {
    bool success;
    std::vector<uint8_t> data;
    uint32_t status;
    std::string error;
};

/**
 * @class Syringe
 * @brief Unified communication layer for all bootrom and payload operations.
 *        Abstracts away transport details (DFU vs recovery vs normal mode).
 *        All bootrom exploits and post-exploit payloads communicate through syringe.
 */
class Syringe {
public:
    Syringe();
    ~Syringe();

    // Device detection and initialization
    bool detectDevice(DeviceManager& manager);
    SyringeTransport currentTransport() const { return mTransport; }
    bool switchTransport(SyringeTransport target);

    // Device info queries
    bool queryCpid(uint32_t& cpid);
    bool queryEcid(uint64_t& ecid);
    bool queryBoardConfig(std::string& config);

    // Memory operations
    bool readMemory(uint64_t addr, uint64_t size, std::vector<uint8_t>& out);
    bool writeMemory(uint64_t addr, const std::vector<uint8_t>& data);

    // Code execution
    bool executeCode(uint64_t addr, const std::vector<uint8_t>& code);
    bool callFunction(uint64_t funcAddr, const std::vector<uint64_t>& args, uint64_t& result);

    // File transfer
    bool sendFile(const std::string& filepath, uint64_t addr = 0);
    bool receiveFile(const std::string& filepath, uint64_t addr, uint64_t size);

    // Generic request/response
    bool sendRequest(const SyringeRequest& req, SyringeResponse& resp);

    /** Chronic-Dev pattern: send_command → getret → optional recv_buffer. */
    bool runCommand(const std::string& command, SyringeResponse& resp, uint64_t recvLength = 0);

    // Connection state
    bool isConnected() const { return mConnected; }
    bool reconnect();
    void disconnect();

private:
    bool mConnected;
    SyringeTransport mTransport;
    DeviceManager* mDeviceManager;

    // Transport-specific implementations
    bool doDFURead(uint64_t addr, uint64_t size, std::vector<uint8_t>& out);
    bool doDFUWrite(uint64_t addr, const std::vector<uint8_t>& data);
    bool doRecoveryRead(uint64_t addr, uint64_t size, std::vector<uint8_t>& out);
    bool doRecoveryWrite(uint64_t addr, const std::vector<uint8_t>& data);

    // Utility helpers
    bool validateRequest(const SyringeRequest& req) const;
    void logRequest(const SyringeRequest& req);
    void logResponse(const SyringeResponse& resp);
    bool fetchCpid(uint32_t& cpid);
    bool fetchEcid(uint64_t& ecid);
    bool runTransportCommand(const std::string& command,
                             SyringeResponse& resp,
                             uint64_t recvLength);
};

} /* namespace PP */

#endif /* SYRINGE_H_ */
