/*
 * DyldSharedCache.cpp
 */

#include "DyldSharedCache.h"

#include "AnalysisPayload.h"
#include "IpswdClient.h"
#include "ToolRunner.h"

#include <sstream>

namespace PP {

DyldSharedCache::DyldSharedCache()
    : m_backend(Backend::None) {}

std::unique_ptr<DyldSharedCache> DyldSharedCache::open(const std::string& path) {
    auto handle = std::unique_ptr<DyldSharedCache>(new DyldSharedCache());
    handle->m_path = path;
    if (handle->loadViaIpswd()) {
        handle->ensureInternalParser();
        return handle;
    }
    if (handle->loadViaIpsw()) {
        handle->ensureInternalParser();
        return handle;
    }
    if (handle->loadViaInternal()) {
        return handle;
    }
    return handle;
}

bool DyldSharedCache::loadViaIpswd() {
    if (!IpswdClient::ping()) {
        return false;
    }

    const IpswdResponse response = IpswdClient::getDscInfo(m_path);
    if (!response.ok() || response.body.empty() || !looksLikeJson(response.body)) {
        return false;
    }

    m_payloadJson = response.body;
    m_backend = Backend::Ipswd;
    return true;
}

bool DyldSharedCache::loadViaIpsw() {
    const std::string ipsw = ToolRunner::findIpswExecutable();
    if (ipsw.empty()) {
        return false;
    }

    std::vector<std::string> argv;
    argv.push_back(ipsw);
    argv.push_back("dyld");
    argv.push_back("info");
    argv.push_back(m_path);
    argv.push_back("--json");
    argv.push_back("--dylibs");

    const CommandResult run = ToolRunner::run(argv);
    if (run.exitCode != 0 || run.stdoutText.empty() || !looksLikeJson(run.stdoutText)) {
        return false;
    }

    m_payloadJson = run.stdoutText;
    m_backend = Backend::Ipsw;
    return true;
}

bool DyldSharedCache::loadViaInternal() {
    try {
        m_parser = std::unique_ptr<DyldCacheParser>(new DyldCacheParser(m_path));
    } catch (const std::exception&) {
        m_parser.reset();
        return false;
    }
    if (!m_parser->isValid()) {
        m_parser.reset();
        return false;
    }
    m_backend = Backend::Internal;
    buildInternalPayloadJson();
    return true;
}

void DyldSharedCache::ensureInternalParser() const {
    if (m_parser) {
        return;
    }
    try {
        m_parser = std::unique_ptr<DyldCacheParser>(new DyldCacheParser(m_path));
        if (!m_parser->isValid()) {
            m_parser.reset();
        }
    } catch (const std::exception&) {
        m_parser.reset();
    }
}

void DyldSharedCache::buildInternalPayloadJson() {
    if (!m_parser) {
        m_payloadJson.clear();
        return;
    }

    std::ostringstream json;
    json << "{\n";
    json << "  \"source\": \"purplepois0n-internal\",\n";
    json << "  \"file_path\": \"" << jsonEscape(m_path) << "\",\n";
    json << "  \"magic\": \"" << jsonEscape(m_parser->getMagicString()) << "\",\n";
    json << "  \"architecture\": \"" << jsonEscape(m_parser->getArchitecture()) << "\",\n";
    json << "  \"uuid\": \"" << jsonEscape(m_parser->getUUID()) << "\",\n";
    json << "  \"supported_variant\": " << (m_parser->isSupportedVariant() ? "true" : "false")
         << ",\n";
    json << "  \"image_count\": " << m_parser->getImageInfos().size() << ",\n";
    json << "  \"mapping_count\": " << m_parser->getMappings().size() << "\n";
    json << "}\n";
    m_payloadJson = json.str();
}

bool DyldSharedCache::isValid() const {
    return m_backend != Backend::None;
}

DyldSharedCache::Backend DyldSharedCache::backend() const {
    return m_backend;
}

const std::string& DyldSharedCache::path() const {
    return m_path;
}

std::string DyldSharedCache::backendName() const {
    switch (m_backend) {
        case Backend::Ipswd:
            return "ipswd";
        case Backend::Ipsw:
            return "ipsw";
        case Backend::Internal:
            return "internal";
        default:
            return "none";
    }
}

const std::string& DyldSharedCache::payloadJson() const {
    return m_payloadJson;
}

std::string DyldSharedCache::magicString() const {
    ensureInternalParser();
    return m_parser ? m_parser->getMagicString() : std::string();
}

std::string DyldSharedCache::architecture() const {
    ensureInternalParser();
    return m_parser ? m_parser->getArchitecture() : std::string();
}

std::string DyldSharedCache::uuid() const {
    ensureInternalParser();
    return m_parser ? m_parser->getUUID() : std::string();
}

bool DyldSharedCache::isSupportedVariant() const {
    ensureInternalParser();
    return m_parser && m_parser->isSupportedVariant();
}

bool DyldSharedCache::isArm32Cache() const {
    ensureInternalParser();
    return m_parser && m_parser->isArm32Cache();
}

bool DyldSharedCache::isArm64Cache() const {
    ensureInternalParser();
    return m_parser && m_parser->isArm64Cache();
}

uint64_t DyldSharedCache::baseAddress() const {
    ensureInternalParser();
    return m_parser ? m_parser->getBaseAddress() : 0;
}

std::vector<DyldCacheMappingInfo> DyldSharedCache::mappings() const {
    ensureInternalParser();
    return m_parser ? m_parser->getMappings() : std::vector<DyldCacheMappingInfo>();
}

std::vector<DyldCacheImageInfo> DyldSharedCache::imageInfos() const {
    ensureInternalParser();
    return m_parser ? m_parser->getImageInfos() : std::vector<DyldCacheImageInfo>();
}

bool DyldSharedCache::extractImage(const std::string& imagePath,
                                  const std::string& outputPath) const {
    ensureInternalParser();
    return m_parser && m_parser->extractImage(imagePath, outputPath);
}

const DyldCacheParser* DyldSharedCache::internalParser() const {
    return m_parser.get();
}

bool DyldSharedCache::writePayloadToFile(const std::string& outPath) const {
    if (m_payloadJson.empty()) {
        return false;
    }
    return writePayloadFile(outPath, m_payloadJson);
}

} /* namespace PP */
