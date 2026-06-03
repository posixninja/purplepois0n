/*
 * MachOBinary.h
 *
 * Opaque Mach-O analysis handle. Prefers ipswd `GET /v1/macho/info`,
 * then `ipsw macho info --json`, then in-tree MachOParser.
 */

#ifndef MACHO_BINARY_H_
#define MACHO_BINARY_H_

#include "MachOParser.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace PP {

class MachOBinary {
public:
    enum class Backend {
        None,
        Ipswd,
        Ipsw,
        Internal
    };

    /** Open and analyze @p path (tries ipswd, then ipsw CLI). */
    static std::unique_ptr<MachOBinary> open(const std::string& path,
                                             MachOArchPreference archPreference =
                                                 MachOArchPreference::Default);

    bool isValid() const;
    Backend backend() const;
    const std::string& path() const;
    /** Backend name for logs: ipswd, ipsw, internal, none. */
    std::string backendName() const;

    /**
     * JSON payload for downstream tools (boogeraids): raw ipsw output when
     * Ipswd/Ipsw backends, otherwise a minimal summary from MachOParser.
     */
    const std::string& payloadJson() const;

    std::string architectureName() const;
    bool is64Bit() const;
    uint32_t loadCommandCount() const;
    uint64_t entryPoint() const;
    std::vector<MachOSegment> segments() const;
    MachOSymtabInfo symtabInfo() const;
    MachODyldInfo dyldInfo() const;
    std::vector<MachOSymbol> symbols() const;

    /** Underlying parser when Backend::Internal (nullptr for ipsw-only). */
    const MachOParser* internalParser() const;

    bool writePayloadToFile(const std::string& outPath) const;

private:
    MachOBinary();

    bool loadViaIpswd(MachOArchPreference archPreference);
    bool loadViaIpsw(MachOArchPreference archPreference);
    bool loadViaInternal(MachOArchPreference archPreference);
    void buildInternalPayloadJson();

    std::string m_path;
    Backend m_backend;
    std::string m_payloadJson;
    std::unique_ptr<MachOParser> m_parser;
};

} /* namespace PP */

#endif /* MACHO_BINARY_H_ */
