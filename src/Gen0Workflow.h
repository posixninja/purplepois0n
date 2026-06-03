/*
 * Gen0Workflow.h
 *
 * Generation 0 (greenpois0n / absinthe) scaffolding: honest status messages
 * and educational tooling without exploit or backup weaponization.
 */

#ifndef GEN0_WORKFLOW_H_
#define GEN0_WORKFLOW_H_

#include <cstdint>
#include <string>
#include "MachOParser.h"

namespace PP {

class DeviceManager;

/** Options for Gen0 scaffold runs. */
struct Gen0Options {
    std::string reportPath;
    std::string backupPath;
};

/**
 * @brief Run Gen 0 jailbreak scaffold for the connected device mode.
 *
 * Connects via DeviceManager where supported and logs what is NOT implemented
 * (bootrom exploits, backup restore, untether). Does not perform exploits.
 */
bool runGen0Jailbreak(DeviceManager& manager,
                      const std::string& targetUDID = "",
                      const Gen0Options& options = Gen0Options());

/**
 * @brief Parse an on-disk iTunes-style backup and print a research summary.
 * @param backupPath Path to backup directory (contains Manifest.plist / Info.plist)
 */
bool analyzeBackup(const std::string& backupPath);

/**
 * @brief Parse a Mach-O binary and print segment/symbol summary (offline research).
 */
bool analyzeBinary(const std::string& binaryPath,
                   MachOArchPreference archPreference = MachOArchPreference::Default,
                   const std::string& payloadJsonPath = "");

/**
 * @brief Parse a dyld shared cache file and print catalog summary (offline research).
 * @param payloadJsonPath If non-empty, write opaque JSON payload (ipsw or internal summary).
 */
bool analyzeDyldCache(const std::string& cachePath,
                      const std::string& payloadJsonPath = "");

} /* namespace PP */

#endif /* GEN0_WORKFLOW_H_ */
