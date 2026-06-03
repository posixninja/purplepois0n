/*
 * DyldSharedCache.h
 *
 * Opaque dyld shared cache handle. Prefers ipswd `GET /v1/dsc/info`,
 * then `ipsw dyld info --json`, then DyldCacheParser. extractImage() uses
 * internal parser when available.
 */

#ifndef DYLD_SHARED_CACHE_H_
#define DYLD_SHARED_CACHE_H_

#include "DyldCacheParser.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace PP {

class DyldSharedCache {
public:
    enum class Backend {
        None,
        Ipswd,
        Ipsw,
        Internal
    };

    static std::unique_ptr<DyldSharedCache> open(const std::string& path);

    bool isValid() const;
    Backend backend() const;
    const std::string& path() const;
    std::string backendName() const;
    const std::string& payloadJson() const;

    std::string magicString() const;
    std::string architecture() const;
    std::string uuid() const;
    bool isSupportedVariant() const;
    bool isArm32Cache() const;
    bool isArm64Cache() const;
    uint64_t baseAddress() const;
    std::vector<DyldCacheMappingInfo> mappings() const;
    std::vector<DyldCacheImageInfo> imageInfos() const;

    bool extractImage(const std::string& imagePath, const std::string& outputPath) const;

    const DyldCacheParser* internalParser() const;
    bool writePayloadToFile(const std::string& outPath) const;

private:
    DyldSharedCache();

    bool loadViaIpswd();
    bool loadViaIpsw();
    bool loadViaInternal();
    void buildInternalPayloadJson();
    void ensureInternalParser() const;

    std::string m_path;
    Backend m_backend;
    std::string m_payloadJson;
    mutable std::unique_ptr<DyldCacheParser> m_parser;
};

} /* namespace PP */

#endif /* DYLD_SHARED_CACHE_H_ */
