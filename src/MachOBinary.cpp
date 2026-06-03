/*
 * MachOBinary.cpp
 */

#include "MachOBinary.h"

#include "AnalysisPayload.h"
#include "IpswdClient.h"
#include "ToolRunner.h"

#include <sstream>

namespace PP {

namespace {

std::string ipswArchFlag(MachOArchPreference preference) {
    switch (preference) {
        case MachOArchPreference::Arm32:
            return "armv7";
        case MachOArchPreference::Arm64:
            return "arm64";
        default:
            return std::string();
    }
}

} /* anonymous */

MachOBinary::MachOBinary()
    : m_backend(Backend::None) {}

std::unique_ptr<MachOBinary> MachOBinary::open(const std::string& path,
                                               MachOArchPreference archPreference) {
    auto handle = std::unique_ptr<MachOBinary>(new MachOBinary());
    handle->m_path = path;
    if (handle->loadViaIpswd(archPreference)) {
        return handle;
    }
    if (handle->loadViaIpsw(archPreference)) {
        return handle;
    }
    if (handle->loadViaInternal(archPreference)) {
        return handle;
    }
    return handle;
}

bool MachOBinary::loadViaIpswd(MachOArchPreference archPreference) {
    if (!IpswdClient::ping()) {
        return false;
    }

    const IpswdResponse response =
        IpswdClient::getMachoInfo(m_path, ipswArchFlag(archPreference));
    if (!response.ok() || response.body.empty() || !looksLikeJson(response.body)) {
        return false;
    }

    m_payloadJson = response.body;
    m_backend = Backend::Ipswd;
    return true;
}

bool MachOBinary::loadViaIpsw(MachOArchPreference archPreference) {
    const std::string ipsw = ToolRunner::findIpswExecutable();
    if (ipsw.empty()) {
        return false;
    }

    std::vector<std::string> argv;
    argv.push_back(ipsw);
    argv.push_back("macho");
    argv.push_back("info");
    argv.push_back(m_path);
    argv.push_back("--json");
    argv.push_back("--header");
    argv.push_back("--loads");
    argv.push_back("--symbols");

    const std::string arch = ipswArchFlag(archPreference);
    if (!arch.empty()) {
        argv.push_back("--arch");
        argv.push_back(arch);
    }

    const CommandResult run = ToolRunner::run(argv);
    if (run.exitCode != 0 || run.stdoutText.empty() || !looksLikeJson(run.stdoutText)) {
        return false;
    }

    m_payloadJson = run.stdoutText;
    m_backend = Backend::Ipsw;
    return true;
}

bool MachOBinary::loadViaInternal(MachOArchPreference archPreference) {
    try {
        m_parser = std::unique_ptr<MachOParser>(new MachOParser(m_path, archPreference));
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

void MachOBinary::buildInternalPayloadJson() {
    if (!m_parser) {
        m_payloadJson.clear();
        return;
    }

    std::ostringstream json;
    json << "{\n";
    json << "  \"source\": \"purplepois0n-internal\",\n";
    json << "  \"file_path\": \"" << jsonEscape(m_path) << "\",\n";
    json << "  \"architecture\": \"" << jsonEscape(m_parser->getArchitectureName()) << "\",\n";
    json << "  \"is_64bit\": " << (m_parser->is64Bit() ? "true" : "false") << ",\n";
    json << "  \"load_commands\": " << m_parser->getLoadCommandCount() << ",\n";
    json << "  \"entry_point\": \"0x" << std::hex << m_parser->getEntryPoint() << std::dec
         << "\",\n";
    json << "  \"segment_count\": " << m_parser->getSegments().size() << ",\n";
    json << "  \"symbol_count\": " << m_parser->getSymbols().size() << "\n";
    json << "}\n";
    m_payloadJson = json.str();
}

bool MachOBinary::isValid() const {
    return m_backend != Backend::None;
}

MachOBinary::Backend MachOBinary::backend() const {
    return m_backend;
}

const std::string& MachOBinary::path() const {
    return m_path;
}

std::string MachOBinary::backendName() const {
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

const std::string& MachOBinary::payloadJson() const {
    return m_payloadJson;
}

std::string MachOBinary::architectureName() const {
    if (m_parser) {
        return m_parser->getArchitectureName();
    }
    return (m_backend == Backend::Ipswd || m_backend == Backend::Ipsw) ? "ipsw-json"
                                                                     : "unknown";
}

bool MachOBinary::is64Bit() const {
    return m_parser ? m_parser->is64Bit() : true;
}

uint32_t MachOBinary::loadCommandCount() const {
    return m_parser ? m_parser->getLoadCommandCount() : 0;
}

uint64_t MachOBinary::entryPoint() const {
    return m_parser ? m_parser->getEntryPoint() : 0;
}

std::vector<MachOSegment> MachOBinary::segments() const {
    return m_parser ? m_parser->getSegments() : std::vector<MachOSegment>();
}

MachOSymtabInfo MachOBinary::symtabInfo() const {
    return m_parser ? m_parser->getSymtabInfo() : MachOSymtabInfo();
}

MachODyldInfo MachOBinary::dyldInfo() const {
    return m_parser ? m_parser->getDyldInfo() : MachODyldInfo();
}

std::vector<MachOSymbol> MachOBinary::symbols() const {
    return m_parser ? m_parser->getSymbols() : std::vector<MachOSymbol>();
}

const MachOParser* MachOBinary::internalParser() const {
    return m_parser.get();
}

bool MachOBinary::writePayloadToFile(const std::string& outPath) const {
    if (m_payloadJson.empty()) {
        return false;
    }
    return writePayloadFile(outPath, m_payloadJson);
}

} /* namespace PP */
