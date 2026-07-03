/*
 * CyanidePayload.h
 *
 * Modernized cyanide: iBoot payload with interactive debugger/REPL.
 * Provides memory inspection, breakpoints, and patch application in iBoot context.
 * Designed to work across all device generations (A5-A13+, S4-S5).
 */

#ifndef CYANIDE_PAYLOAD_H_
#define CYANIDE_PAYLOAD_H_

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "primitives/PrimitiveTypes.h"

namespace PP {

using primitives::ExecutionContext;

class DeviceManager;
class Syringe;

enum class CyanideCommand {
    Memory,
    Breakpoint,
    Patch,
    Symbol,
    Dump,
    Quit
};

/**
 * @class CyanidePayload
 * @brief iBoot interactive debugger payload.
 *        Provides REPL for memory inspection, patching, breakpoints.
 */
class CyanidePayload {
public:
    CyanidePayload();
    ~CyanidePayload();

    const char* name() const { return "cyanide"; }

    bool load(DeviceManager& manager, const ExecutionContext& context);
    bool isLoaded() const { return mLoaded; }

    // Interactive REPL
    bool runREPL();
    bool executeCommand(const std::string& cmd);

    // Memory operations
    bool readMemory(uint64_t addr, uint64_t size, std::vector<uint8_t>& out);
    bool writeMemory(uint64_t addr, const std::vector<uint8_t>& data);

    // Breakpoint support
    bool setBreakpoint(uint64_t addr);
    bool removeBreakpoint(uint64_t addr);

    // Patching
    bool patchMemory(uint64_t addr, const std::vector<uint8_t>& pattern,
                     const std::vector<uint8_t>& replacement);

    // Symbol resolution
    bool resolveSymbol(const std::string& symbol, uint64_t& addr);
    bool findFunction(const std::string& name, uint64_t& addr);

private:
    bool mLoaded;
    DeviceManager* mManager;
    std::shared_ptr<Syringe> mSyringe;

    // Breakpoints and active patches
    std::vector<uint64_t> mBreakpoints;
    std::vector<std::pair<uint64_t, std::vector<uint8_t>>> mActivePatches;

    // Command parser and dispatcher
    bool parseCommand(const std::string& line, CyanideCommand& cmd, std::vector<std::string>& args);
    void displayHelp();
};

} /* namespace PP */

#endif /* CYANIDE_PAYLOAD_H_ */
