/*
 * IpaSignHelper.cpp
 */

#include "IpaSignHelper.h"
#include "primitives/CodesignDelegate.h"
#include "primitives/CodesignTypes.h"
#include "ToolRunner.h"
#include "Logger.h"

#include <cstdlib>
#include <dirent.h>
#include <vector>

namespace PP {

namespace {

std::string findAppBundleInPayload(const std::string& payloadDir) {
    DIR* dir = opendir(payloadDir.c_str());
    if (dir == nullptr) {
        return std::string();
    }
    std::string found;
    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        const std::string name = entry->d_name;
        if (name.size() > 4 && name.compare(name.size() - 4, 4, ".app") == 0) {
            found = payloadDir + "/" + name;
            break;
        }
    }
    closedir(dir);
    return found;
}

std::string defaultSignedIpaPath(const std::string& inputIpa) {
    if (inputIpa.size() > 4 && inputIpa.compare(inputIpa.size() - 4, 4, ".ipa") == 0) {
        return inputIpa.substr(0, inputIpa.size() - 4) + "-signed.ipa";
    }
    return inputIpa + "-signed.ipa";
}

} /* anonymous */

std::string signIpa(const primitives::CodesignOptions& opts,
                    const std::string& ipaPath,
                    const std::string& outputIpaPath,
                    bool allowMutation) {
    if (ipaPath.empty()) {
        return std::string();
    }

    const std::string workDir = "/tmp/pp-ipa-sign";
    const std::string extractDir = workDir + "/extract";
    const std::string payloadDir = extractDir + "/Payload";

    std::vector<std::string> cleanArgv;
    cleanArgv.push_back("/bin/rm");
    cleanArgv.push_back("-rf");
    cleanArgv.push_back(workDir);
    ToolRunner::run(cleanArgv);

    std::vector<std::string> mkdirArgv;
    mkdirArgv.push_back("/bin/mkdir");
    mkdirArgv.push_back("-p");
    mkdirArgv.push_back(extractDir);
    if (ToolRunner::run(mkdirArgv).exitCode != 0) {
        Logger::error("  [Codesign] failed to create work directory");
        return std::string();
    }

    std::vector<std::string> unzipArgv;
    unzipArgv.push_back("/usr/bin/unzip");
    unzipArgv.push_back("-q");
    unzipArgv.push_back("-o");
    unzipArgv.push_back(ipaPath);
    unzipArgv.push_back("-d");
    unzipArgv.push_back(extractDir);
    Logger::info("  [Codesign] extracting IPA...");
    if (ToolRunner::run(unzipArgv).exitCode != 0) {
        Logger::error("  [Codesign] unzip failed for " + ipaPath);
        return std::string();
    }

    const std::string appBundle = findAppBundleInPayload(payloadDir);
    if (appBundle.empty()) {
        Logger::error("  [Codesign] no Payload/*.app found in IPA");
        return std::string();
    }

    primitives::CodesignOptions appOpts = opts;
    appOpts.overwrite = true;
    if (appOpts.bundleId.empty()) {
        const size_t slash = appBundle.find_last_of('/');
        if (slash != std::string::npos) {
            std::string name = appBundle.substr(slash + 1);
            if (name.size() > 4) {
                appOpts.bundleId = name.substr(0, name.size() - 4);
            }
        }
    }

    const primitives::PrimitiveResult signResult =
        primitives::CodesignDelegate::signPath(appOpts, appBundle, allowMutation);
    if (signResult != primitives::PrimitiveResult::Success) {
        return std::string();
    }

    const std::string outIpa =
        outputIpaPath.empty() ? defaultSignedIpaPath(ipaPath) : outputIpaPath;

    std::vector<std::string> zipArgv;
    zipArgv.push_back("/bin/sh");
    zipArgv.push_back("-c");
    zipArgv.push_back("cd '" + extractDir + "' && /usr/bin/zip -qry '" + outIpa + "' Payload");
    const CommandResult zipResult = ToolRunner::run(zipArgv);
    if (zipResult.exitCode != 0) {
        Logger::error("  [Codesign] failed to repack IPA");
        return std::string();
    }

    Logger::info("  [Codesign] signed IPA → " + outIpa);
    return outIpa;
}

} /* namespace PP */
