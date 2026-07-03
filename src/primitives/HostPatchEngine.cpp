/*
 * HostPatchEngine.cpp
 */

#include "primitives/HostPatchEngine.h"
#include "EnvUtil.h"
#include "MachOBinary.h"
#include "MachOParser.h"
#include "Logger.h"
#include "ToolRunner.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

namespace PP {
namespace primitives {

namespace {

struct PatchEntry {
    uint64_t offset = 0;
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> expect;
    std::string kind;
    std::string label;
};

bool offsetInExecutableSection(MachOParser& parser, uint64_t offset) {
    const std::vector<MachOSection> sections = parser.getSections();
    for (size_t i = 0; i < sections.size(); ++i) {
        const MachOSection& sec = sections[i];
        if (offset < sec.offset || offset >= sec.offset + sec.size) {
            continue;
        }
        if (sec.segname == "__TEXT_EXEC") {
            return true;
        }
        if (sec.sectname == "__text") {
            return true;
        }
        if (sec.flags & 0x80000000u) {
            return true;
        }
    }
    return false;
}

void scanDataStringPatches(MachOParser& parser, const std::string& kernelPath) {
    static const char* needles[] = {
        "com.apple.os.update-",
        "launch-failure-unsupported-ancillary",
        "Signed System Volume",
        "Seatbelt sandbox policy",
    };
    std::ifstream in(kernelPath, std::ios::binary);
    if (!in) {
        return;
    }
    std::vector<uint8_t> image((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    for (size_t n = 0; n < sizeof(needles) / sizeof(needles[0]); ++n) {
        const char* needle = needles[n];
        const size_t needleLen = std::strlen(needle);
        for (size_t pos = 0; pos + needleLen <= image.size(); ++pos) {
            if (std::memcmp(image.data() + pos, needle, needleLen) != 0) {
                continue;
            }
            if (offsetInExecutableSection(parser, pos)) {
                continue;
            }
            Logger::info(std::string("  [Patch] data candidate @ 0x") + [&]() {
                std::ostringstream oss;
                oss << std::hex << pos;
                return oss.str();
            }() + " label=" + needle + " (single-byte neuter)");
            break;
        }
    }
}

bool readFileBytes(const std::string& path, std::vector<uint8_t>* out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size < 0) {
        return false;
    }
    in.seekg(0, std::ios::beg);
    out->resize(static_cast<size_t>(size));
    if (size > 0) {
        in.read(reinterpret_cast<char*>(out->data()), size);
    }
    return static_cast<bool>(in);
}

bool writeFileBytes(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }
    return static_cast<bool>(out);
}

size_t skipWs(const std::string& s, size_t pos) {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
        ++pos;
    }
    return pos;
}

bool parseHexBytePair(const std::string& hex, size_t* pos, uint8_t* out) {
    if (*pos + 1 >= hex.size()) {
        return false;
    }
    const char pair[3] = {hex[*pos], hex[*pos + 1], '\0'};
    char* end = nullptr;
    const unsigned long value = std::strtoul(pair, &end, 16);
    if (end == pair || value > 0xFFu) {
        return false;
    }
    *out = static_cast<uint8_t>(value);
    *pos += 2;
    return true;
}

bool parseHexString(const std::string& hex, std::vector<uint8_t>* out) {
    size_t pos = 0;
    while (pos < hex.size()) {
        if (hex[pos] == ' ' || hex[pos] == '\t') {
            ++pos;
            continue;
        }
        uint8_t byte = 0;
        if (!parseHexBytePair(hex, &pos, &byte)) {
            return false;
        }
        out->push_back(byte);
    }
    return !out->empty();
}

bool findKey(const std::string& json, const std::string& key, size_t start, size_t* valueStart) {
    const std::string needle = "\"" + key + "\"";
    const size_t pos = json.find(needle, start);
    if (pos == std::string::npos) {
        return false;
    }
    size_t cursor = pos + needle.size();
    cursor = skipWs(json, cursor);
    if (cursor >= json.size() || json[cursor] != ':') {
        return false;
    }
    ++cursor;
    cursor = skipWs(json, cursor);
    *valueStart = cursor;
    return true;
}

bool parseNumberAt(const std::string& json, size_t pos, uint64_t* out) {
    size_t end = pos;
    while (end < json.size() &&
           (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == 'x' ||
            json[end] == 'X' || (json[end] >= 'a' && json[end] <= 'f') ||
            (json[end] >= 'A' && json[end] <= 'F'))) {
        ++end;
    }
    if (end == pos) {
        return false;
    }
    const std::string token = json.substr(pos, end - pos);
    std::stringstream ss;
    if (token.find("0x") == 0 || token.find("0X") == 0) {
        ss << std::hex << token.substr(2);
    } else {
        ss << token;
    }
    ss >> *out;
    return !ss.fail();
}

bool parseQuotedStringAt(const std::string& json, size_t pos, std::string* out) {
    if (pos >= json.size() || json[pos] != '"') {
        return false;
    }
    ++pos;
    std::string value;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            value.push_back(json[pos + 1]);
            pos += 2;
            continue;
        }
        value.push_back(json[pos]);
        ++pos;
    }
    if (pos >= json.size() || json[pos] != '"') {
        return false;
    }
    *out = value;
    return true;
}

bool parseBytesArray(const std::string& json, size_t pos, std::vector<uint8_t>* out) {
    if (pos >= json.size() || json[pos] != '[') {
        return false;
    }
    ++pos;
    while (pos < json.size()) {
        pos = skipWs(json, pos);
        if (pos < json.size() && json[pos] == ']') {
            return !out->empty();
        }
        uint64_t byte = 0;
        if (!parseNumberAt(json, pos, &byte)) {
            return false;
        }
        out->push_back(static_cast<uint8_t>(byte & 0xFFu));
        while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos]))) {
            ++pos;
        }
        pos = skipWs(json, pos);
        if (pos < json.size() && json[pos] == ',') {
            ++pos;
            continue;
        }
        if (pos < json.size() && json[pos] == ']') {
            return !out->empty();
        }
    }
    return false;
}

bool parsePatchObject(const std::string& json, size_t pos, PatchEntry* entry) {
    if (pos >= json.size() || json[pos] != '{') {
        return false;
    }
    size_t cursor = pos + 1;
    bool haveOffset = false;
    bool haveBytes = false;
    while (cursor < json.size()) {
        cursor = skipWs(json, cursor);
        if (cursor < json.size() && json[cursor] == '}') {
            break;
        }
        size_t keyPos = 0;
        if (!findKey(json, "offset", cursor, &keyPos)) {
            const size_t symPos = json.find("\"symbol\"", cursor);
            if (symPos != std::string::npos && symPos < json.find('}', cursor)) {
                Logger::warn("  [Patch] symbol patches require offline analysis — use offset");
            }
            break;
        }
        uint64_t offset = 0;
        if (!parseNumberAt(json, keyPos, &offset)) {
            return false;
        }
        entry->offset = offset;
        haveOffset = true;

        size_t hexStart = 0;
        if (findKey(json, "hex", cursor, &hexStart)) {
            std::string hex;
            if (!parseQuotedStringAt(json, hexStart, &hex)) {
                return false;
            }
            if (!parseHexString(hex, &entry->bytes)) {
                return false;
            }
            haveBytes = true;
        } else if (findKey(json, "bytes", cursor, &hexStart)) {
            if (!parseBytesArray(json, hexStart, &entry->bytes)) {
                return false;
            }
            haveBytes = true;
        }

        size_t expectStart = 0;
        if (findKey(json, "expect_hex", cursor, &expectStart)) {
            std::string expectHex;
            if (!parseQuotedStringAt(json, expectStart, &expectHex)) {
                return false;
            }
            if (!parseHexString(expectHex, &entry->expect)) {
                return false;
            }
        }

        size_t kindStart = 0;
        if (findKey(json, "kind", cursor, &kindStart)) {
            parseQuotedStringAt(json, kindStart, &entry->kind);
        }
        size_t labelStart = 0;
        if (findKey(json, "label", cursor, &labelStart)) {
            parseQuotedStringAt(json, labelStart, &entry->label);
        }

        cursor = json.find('}', cursor);
        if (cursor == std::string::npos) {
            return false;
        }
        ++cursor;
        break;
    }
    return haveOffset && haveBytes;
}

bool loadPatchProfile(const std::string& path, std::vector<PatchEntry>* patches) {
    std::ifstream in(path);
    if (!in) {
        Logger::error("  [Patch] cannot open profile: " + path);
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string json = ss.str();

    size_t arrayStart = 0;
    if (!findKey(json, "patches", 0, &arrayStart)) {
        Logger::error("  [Patch] profile missing \"patches\" array");
        return false;
    }
    if (arrayStart >= json.size() || json[arrayStart] != '[') {
        Logger::error("  [Patch] invalid patches array");
        return false;
    }

    size_t cursor = arrayStart + 1;
    while (cursor < json.size()) {
        cursor = skipWs(json, cursor);
        if (cursor < json.size() && json[cursor] == ']') {
            break;
        }
        PatchEntry entry;
        if (!parsePatchObject(json, cursor, &entry)) {
            Logger::error("  [Patch] failed to parse patch entry");
            return false;
        }
        patches->push_back(entry);
        cursor = json.find('}', cursor);
        if (cursor == std::string::npos) {
            return false;
        }
        ++cursor;
        cursor = skipWs(json, cursor);
        if (cursor < json.size() && json[cursor] == ',') {
            ++cursor;
        }
    }

    if (patches->empty()) {
        Logger::error("  [Patch] profile contains no patches");
        return false;
    }
    return true;
}

void logIpswKernelVersion(const std::string& kernelcachePath) {
    const std::string ipsw = ToolRunner::findIpswExecutable();
    if (ipsw.empty()) {
        Logger::info("  [Patch] ipsw not found — skip kernel version probe");
        return;
    }
    std::vector<std::string> argv;
    argv.push_back(ipsw);
    argv.push_back("kernel");
    argv.push_back("version");
    argv.push_back(kernelcachePath);
    argv.push_back("--json");
    const CommandResult run = ToolRunner::run(argv);
    if (run.exitCode != 0 || run.stdoutText.empty()) {
        Logger::info("  [Patch] ipsw kernel version unavailable");
        return;
    }
    const size_t maxLen = 512;
    const std::string snippet = run.stdoutText.size() > maxLen
                                    ? run.stdoutText.substr(0, maxLen) + "..."
                                    : run.stdoutText;
    Logger::info("  [Patch] ipsw kernel version: " + snippet);
}

} /* anonymous */

std::string resolveKernelcachePath(const ExecutionContext& context) {
    if (!context.kernelcachePath.empty()) {
        return context.kernelcachePath;
    }
    return PP::envOrEmpty("PURPLEPOIS0N_KERNELCACHE");
}

std::string resolvePatchProfilePath(const ExecutionContext& context) {
    if (!context.patchProfilePath.empty()) {
        return context.patchProfilePath;
    }
    return PP::envOrEmpty("PURPLEPOIS0N_PATCH_PROFILE");
}

std::string resolvePatchOutPath(const ExecutionContext& context) {
    if (!context.patchOutPath.empty()) {
        return context.patchOutPath;
    }
    return PP::envOrEmpty("PURPLEPOIS0N_PATCH_OUT");
}

PrimitiveResult runHostPatchfind(const ExecutionContext& context) {
    const std::string path = resolveKernelcachePath(context);
    if (path.empty()) {
        Logger::info("  [Patch] set --kernelcache or PURPLEPOIS0N_KERNELCACHE for host patchfind");
        return PrimitiveResult::Success;
    }

    Logger::info("  [Patch] patchfind on " + path);
    std::unique_ptr<MachOBinary> binary = MachOBinary::open(path);
    if (binary == nullptr || !binary->isValid()) {
        Logger::error("  [Patch] Mach-O open failed");
        return PrimitiveResult::Failed;
    }

    Logger::info("  [Patch] backend: " + binary->backendName());
    Logger::info("  [Patch] arch: " + binary->architectureName() +
                 (binary->is64Bit() ? " (64-bit)" : " (32-bit)"));
    Logger::info("  [Patch] load commands: " + std::to_string(binary->loadCommandCount()));
    Logger::info("  [Patch] entry: 0x" + [&]() {
        std::ostringstream oss;
        oss << std::hex << binary->entryPoint();
        return oss.str();
    }());

    const std::vector<MachOSegment> segments = binary->segments();
    Logger::info("  [Patch] segments: " + std::to_string(segments.size()));
    for (size_t i = 0; i < segments.size() && i < 8; ++i) {
        Logger::info("  [Patch]   " + segments[i].segname + " vm=0x" + [&]() {
            std::ostringstream oss;
            oss << std::hex << segments[i].vmaddr;
            return oss.str();
        }());
    }

    const MachOSymtabInfo symtab = binary->symtabInfo();
    if (symtab.present && symtab.nsyms > 0) {
        Logger::info("  [Patch] symtab symbols: " + std::to_string(symtab.nsyms));
    }

    logIpswKernelVersion(path);

    if (binary->internalParser() != nullptr) {
        MachOParser* parser = const_cast<MachOParser*>(binary->internalParser());
        if (PP::envFlagEnabled("PURPLEPOIS0N_DATA_PATCHFIND")) {
            Logger::info("  [Patch] data-only cstring candidates:");
            scanDataStringPatches(*parser, path);
        }
    }

    return PrimitiveResult::Success;
}

PrimitiveResult runHostPatchApply(ExecutionContext& context) {
    const std::string inputPath = resolveKernelcachePath(context);
    const std::string profilePath = resolvePatchProfilePath(context);
    std::string outputPath = resolvePatchOutPath(context);

    if (inputPath.empty() || profilePath.empty()) {
        Logger::info("  [Patch] apply requires --kernelcache and --patch-profile");
        return PrimitiveResult::Success;
    }
    if (outputPath.empty()) {
        outputPath = inputPath + ".patched";
    }

    if (!context.allowMutation) {
        Logger::info("  [Patch] would apply " + profilePath + " → " + outputPath);
        return PrimitiveResult::Success;
    }

    if (!exploitPluginsEnabled()) {
        Logger::warn("  [Patch] apply requires make plugins");
        return PrimitiveResult::PluginDisabled;
    }

    std::vector<PatchEntry> patches;
    if (!loadPatchProfile(profilePath, &patches)) {
        return PrimitiveResult::Failed;
    }

    std::vector<uint8_t> image;
    if (!readFileBytes(inputPath, &image)) {
        Logger::error("  [Patch] read failed: " + inputPath);
        return PrimitiveResult::Failed;
    }

    std::unique_ptr<MachOParser> parserForGuard(new MachOParser(inputPath));
    const bool dataOnly = PP::envFlagEnabled("PURPLEPOIS0N_DATA_ONLY_PATCH");

    for (size_t i = 0; i < patches.size(); ++i) {
        const PatchEntry& patch = patches[i];
        if (dataOnly && parserForGuard != nullptr && parserForGuard->isValid() &&
            offsetInExecutableSection(*parserForGuard, patch.offset)) {
            Logger::error("  [Patch] data-only mode rejects executable offset 0x" + [&]() {
                std::ostringstream oss;
                oss << std::hex << patch.offset;
                return oss.str();
            }());
            return PrimitiveResult::Failed;
        }
        if (patch.offset + patch.bytes.size() > image.size()) {
            Logger::error("  [Patch] patch " + std::to_string(i) + " out of range");
            return PrimitiveResult::Failed;
        }
        if (!patch.expect.empty()) {
            if (patch.expect.size() > patch.bytes.size()) {
                Logger::error("  [Patch] patch " + std::to_string(i) + " expect_hex longer than patch");
                return PrimitiveResult::Failed;
            }
            for (size_t b = 0; b < patch.expect.size(); ++b) {
                if (image[patch.offset + b] != patch.expect[b]) {
                    Logger::warn("  [Patch] patch " + std::to_string(i) +
                                 " expect_hex mismatch @ byte " + std::to_string(b));
                    break;
                }
            }
        }
        for (size_t b = 0; b < patch.bytes.size(); ++b) {
            image[patch.offset + b] = patch.bytes[b];
        }
        Logger::info("  [Patch] applied " + std::to_string(patch.bytes.size()) + " bytes @ 0x" +
                     [&]() {
                         std::ostringstream oss;
                         oss << std::hex << patch.offset;
                         return oss.str();
                     }());
    }

    if (!writeFileBytes(outputPath, image)) {
        Logger::error("  [Patch] write failed: " + outputPath);
        return PrimitiveResult::Failed;
    }

    context.patchOutPath = outputPath;
    Logger::info("  [Patch] wrote patched kernelcache: " + outputPath);
    return PrimitiveResult::Success;
}

} /* namespace primitives */
} /* namespace PP */
