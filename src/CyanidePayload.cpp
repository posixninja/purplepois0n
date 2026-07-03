/*
 * CyanidePayload.cpp
 */

#include "CyanidePayload.h"
#include "DeviceManager.h"
#include "Syringe.h"
#include "Logger.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace PP {

CyanidePayload::CyanidePayload() : mLoaded(false), mManager(nullptr) {}

CyanidePayload::~CyanidePayload() {}

bool CyanidePayload::load(DeviceManager& manager, const ExecutionContext& context) {
    Logger::info("Loading cyanide iBoot debugger payload...");
    mManager = &manager;

    mSyringe = std::make_shared<Syringe>();
    if (!mSyringe->detectDevice(manager)) {
        Logger::error("Cyanide: Failed to detect device via syringe");
        return false;
    }

    // TODO: Load cyanide binary from context.ipswPath or bundled resources
    // TODO: Validate device compatibility (all A5+ and S4+)

    mLoaded = true;
    Logger::debug("Cyanide loaded successfully");
    return true;
}

bool CyanidePayload::runREPL() {
    if (!mLoaded) {
        Logger::error("Cyanide not loaded");
        return false;
    }

    Logger::info("Entering cyanide REPL. Type 'help' for commands.");
    std::string line;

    while (true) {
        std::cout << "cyanide> " << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        if (!executeCommand(line)) {
            break;
        }
    }

    return true;
}

bool CyanidePayload::executeCommand(const std::string& cmd) {
    CyanideCommand command;
    std::vector<std::string> args;

    if (!parseCommand(cmd, command, args)) {
        Logger::warn("Unknown command");
        return true;
    }

    switch (command) {
        case CyanideCommand::Memory: {
            if (args.empty()) {
                Logger::warn("Usage: mem <read|write> <addr> [size|data]");
                return true;
            }
            if (args[0] == "read" && args.size() >= 2) {
                uint64_t addr = std::stoull(args[1], nullptr, 16);
                uint64_t size = args.size() >= 3 ? std::stoull(args[2], nullptr, 16) : 0x100;
                std::vector<uint8_t> data;
                if (readMemory(addr, size, data)) {
                    std::ostringstream oss;
                    oss << "Read " << size << " bytes from 0x" << std::hex << addr;
                    Logger::info(oss.str());
                }
            } else if (args[0] == "write" && args.size() >= 3) {
                uint64_t addr = std::stoull(args[1], nullptr, 16);
                Logger::info("Write not yet implemented");
            }
            return true;
        }

        case CyanideCommand::Breakpoint: {
            if (args.empty() || args.size() < 2) {
                Logger::warn("Usage: bp <set|clear> <addr>");
                return true;
            }
            uint64_t addr = std::stoull(args[1], nullptr, 16);
            if (args[0] == "set") {
                setBreakpoint(addr);
            } else if (args[0] == "clear") {
                removeBreakpoint(addr);
            }
            return true;
        }

        case CyanideCommand::Patch: {
            if (args.size() < 3) {
                Logger::warn("Usage: patch <addr> <pattern> <replacement>");
                return true;
            }
            Logger::info("Patch command not yet implemented");
            return true;
        }

        case CyanideCommand::Symbol: {
            if (args.empty()) {
                Logger::warn("Usage: sym <symbol_name>");
                return true;
            }
            uint64_t addr;
            if (resolveSymbol(args[0], addr)) {
                std::ostringstream oss;
                oss << args[0] << " @ 0x" << std::hex << addr;
                Logger::info(oss.str());
            }
            return true;
        }

        case CyanideCommand::Dump: {
            if (args.empty()) {
                Logger::warn("Usage: dump <addr> [size]");
                return true;
            }
            uint64_t addr = std::stoull(args[0], nullptr, 16);
            uint64_t size = args.size() >= 2 ? std::stoull(args[1], nullptr, 16) : 0x100;
            std::vector<uint8_t> data;
            if (readMemory(addr, size, data)) {
                for (size_t i = 0; i < data.size(); i += 16) {
                    printf("0x%llx: ", addr + i);
                    for (size_t j = i; j < std::min(i + 16, data.size()); j++) {
                        printf("%02x ", data[j]);
                    }
                    printf("\n");
                }
            }
            return true;
        }

        case CyanideCommand::Quit:
            return false;
    }

    return true;
}

bool CyanidePayload::readMemory(uint64_t addr, uint64_t size, std::vector<uint8_t>& out) {
    if (!mSyringe || !mSyringe->isConnected()) {
        Logger::error("Cyanide: Syringe not connected for memory read");
        return false;
    }

    if (!mSyringe->readMemory(addr, size, out)) {
        Logger::error("Cyanide: Failed to read memory via syringe");
        return false;
    }

    return true;
}

bool CyanidePayload::writeMemory(uint64_t addr, const std::vector<uint8_t>& data) {
    if (!mSyringe || !mSyringe->isConnected()) {
        Logger::error("Cyanide: Syringe not connected for memory write");
        return false;
    }

    if (!mSyringe->writeMemory(addr, data)) {
        Logger::error("Cyanide: Failed to write memory via syringe");
        return false;
    }

    return true;
}

bool CyanidePayload::setBreakpoint(uint64_t addr) {
    mBreakpoints.push_back(addr);
    std::ostringstream oss;
    oss << "Breakpoint set at 0x" << std::hex << addr;
    Logger::info(oss.str());
    return true;
}

bool CyanidePayload::removeBreakpoint(uint64_t addr) {
    auto it = std::find(mBreakpoints.begin(), mBreakpoints.end(), addr);
    if (it != mBreakpoints.end()) {
        mBreakpoints.erase(it);
        std::ostringstream oss;
        oss << "Breakpoint removed from 0x" << std::hex << addr;
        Logger::info(oss.str());
        return true;
    }
    return false;
}

bool CyanidePayload::patchMemory(uint64_t addr, const std::vector<uint8_t>& pattern,
                                 const std::vector<uint8_t>& replacement) {
    std::vector<uint8_t> current;
    if (!readMemory(addr, pattern.size(), current)) {
        return false;
    }

    if (current != pattern) {
        std::ostringstream oss;
        oss << "Pattern mismatch at 0x" << std::hex << addr;
        Logger::warn(oss.str());
        return false;
    }

    if (!writeMemory(addr, replacement)) {
        return false;
    }

    mActivePatches.push_back({addr, pattern});
    {
        std::ostringstream oss;
        oss << "Patched 0x" << std::hex << addr << " (" << replacement.size() << " bytes)";
        Logger::info(oss.str());
    }
    return true;
}

bool CyanidePayload::resolveSymbol(const std::string& symbol, uint64_t& addr) {
    // TODO: Use device's symbol table to resolve symbol to address
    Logger::warn("Symbol resolution not yet implemented");
    return false;
}

bool CyanidePayload::findFunction(const std::string& name, uint64_t& addr) {
    // TODO: Search iBoot binary for function by name
    Logger::warn("Function finding not yet implemented");
    return false;
}

bool CyanidePayload::parseCommand(const std::string& line, CyanideCommand& cmd,
                                   std::vector<std::string>& args) {
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return false;
    }

    const std::string& verb = tokens[0];
    args.assign(tokens.begin() + 1, tokens.end());

    if (verb == "help") {
        displayHelp();
        return true;
    } else if (verb == "mem") {
        cmd = CyanideCommand::Memory;
    } else if (verb == "bp") {
        cmd = CyanideCommand::Breakpoint;
    } else if (verb == "patch") {
        cmd = CyanideCommand::Patch;
    } else if (verb == "sym") {
        cmd = CyanideCommand::Symbol;
    } else if (verb == "dump") {
        cmd = CyanideCommand::Dump;
    } else if (verb == "quit" || verb == "exit") {
        cmd = CyanideCommand::Quit;
    } else {
        return false;
    }

    return true;
}

void CyanidePayload::displayHelp() {
    printf("Cyanide iBoot Debugger Commands:\n");
    printf("  mem read <addr> [size]      - Read memory\n");
    printf("  mem write <addr> <data>     - Write memory\n");
    printf("  bp set <addr>               - Set breakpoint\n");
    printf("  bp clear <addr>             - Clear breakpoint\n");
    printf("  patch <addr> <pat> <repl>   - Patch memory\n");
    printf("  sym <symbol>                - Resolve symbol\n");
    printf("  dump <addr> [size]          - Dump memory as hex\n");
    printf("  quit                        - Exit debugger\n");
}

} /* namespace PP */
