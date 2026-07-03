/*
 * AppleCvsDatabase.h
 *
 * Integration with Apple's historical CVS/Git repositories for patch source discovery.
 * Correlates iOS kernel source with identified patch sites to provide historical context
 * and verify patch correctness across iOS versions.
 */

#ifndef APPLE_CVS_DATABASE_H_
#define APPLE_CVS_DATABASE_H_

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace PP {

/**
 * @struct PatchSource
 * @brief Historical Apple source information for a patch.
 */
struct PatchSource {
    std::string commitHash;
    std::string commitMessage;
    std::string author;
    std::string date;
    std::string fileChanged;
    uint32_t startLine;
    uint32_t endLine;
    std::string sourceURL;  // GitHub or other public repo
};

/**
 * @struct PatchHistory
 * @brief Track when a vulnerability was fixed across iOS versions.
 */
struct PatchHistory {
    std::string patchName;
    std::string cveId;
    std::string appleSecurityBulletin;
    std::vector<std::string> affectedVersions;  // "iOS 14.0", "iOS 14.1", etc.
    std::vector<std::string> fixedInVersions;   // "iOS 14.2", "iOS 14.3", etc.
    std::string discoveredBy;
    std::string publicDisclosureDate;
};

/**
 * @class AppleCvsDatabase
 * @brief Historical Apple source code and patch metadata.
 *        Correlates discovered patches with Apple's public source releases
 *        and security bulletins.
 */
class AppleCvsDatabase {
public:
    AppleCvsDatabase();
    ~AppleCvsDatabase();

    // Database initialization
    bool initialize();
    bool loadFromLocalCache();
    bool downloadLatestDatabase();

    // Patch source discovery
    bool findPatchSource(const std::string& patchName, PatchSource& source);
    bool findPatchHistory(const std::string& patchName, PatchHistory& history);

    // CVE correlation
    bool findByCVE(const std::string& cveId, std::vector<PatchSource>& results);
    bool findBySecurityBulletin(const std::string& bulletin,
                                std::vector<PatchHistory>& results);

    // Verification
    bool verifyPatchAgainstSource(const std::string& patchName,
                                  const std::vector<uint8_t>& pattern,
                                  const std::vector<uint8_t>& replacement);

    // Educational features
    bool getPatchExplanation(const std::string& patchName, std::string& explanation);
    bool getHistoricalContext(const std::string& patchName, std::string& context);

private:
    bool mInitialized;

    // In-memory database
    std::map<std::string, PatchSource> mPatchSources;
    std::map<std::string, PatchHistory> mPatchHistories;
    std::map<std::string, std::string> mCVEMapping;

    // Internal helpers
    bool loadKernelSourceDatabase();
    bool loadSecurityBulletins();
    bool loadCVEMappings();
    bool downloadFromGitHub();
};

} /* namespace PP */

#endif /* APPLE_CVS_DATABASE_H_ */
