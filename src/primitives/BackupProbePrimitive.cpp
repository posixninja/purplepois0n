/*
 * BackupProbePrimitive.cpp
 */

#include "primitives/BackupProbePrimitive.h"
#include "MobileBackup.h"
#include "Logger.h"

#include <sstream>

namespace PP {
namespace primitives {

const char* BackupProbePrimitive::name() const { return "backup-probe"; }

PrimitiveCategory BackupProbePrimitive::category() const { return PrimitiveCategory::Sandbox; }

std::vector<PrimitiveOperation> BackupProbePrimitive::supportedOperations() const {
    return std::vector<PrimitiveOperation>{PrimitiveOperation::Probe};
}

std::vector<DeviceState> BackupProbePrimitive::requiredDeviceStates() const {
    return std::vector<DeviceState>{DeviceState::Unknown,
                                    DeviceState::Normal,
                                    DeviceState::Recovery,
                                    DeviceState::DFU};
}

bool BackupProbePrimitive::canRun(const ExecutionContext& context) const {
    return !context.backupPath.empty();
}

PrimitiveResult BackupProbePrimitive::execute(ExecutionContext& context) {
    Logger::info("  [Backup] analyzing offline backup (parse only): " + context.backupPath);

    try {
        MobileBackup backup(context.backupPath);
        if (!backup.isValid()) {
            Logger::error("  [Backup] invalid or unreadable backup tree");
            return PrimitiveResult::Failed;
        }

        const std::vector<std::string> domains = backup.getDomains();
        const MobileBackup::Stats stats = backup.getStats();

        Logger::info(std::string("  [Backup]   manifest: ") + backup.getManifestTypeName());
        Logger::info("  [Backup]   iOS " + backup.getIOSVersion() + " — " + backup.getDeviceName());
        Logger::info(std::string("  [Backup]   encrypted: ") + (backup.isEncrypted() ? "yes" : "no"));
        Logger::info("  [Backup]   domains: " + std::to_string(domains.size()));
        Logger::info("  [Backup]   entries: " + std::to_string(stats.totalEntries) +
                     " (metadata " + std::to_string(stats.entriesWithMetadata) + ")");

        if (backup.isEncrypted()) {
            Logger::warn("  [Backup]   content decrypt NOT implemented — metadata only");
        }
        Logger::info("  [Backup]   absinthe-era restore/staging — NOT in-tree");

        return PrimitiveResult::Success;
    } catch (const std::exception& e) {
        Logger::error(std::string("  [Backup] analysis failed: ") + e.what());
        return PrimitiveResult::Failed;
    }
}

} /* namespace primitives */
} /* namespace PP */
