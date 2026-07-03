/*
 * JsonMini.cpp
 */

#include "devicetree/JsonMini.h"

#include <cctype>
#include <cstdlib>
#include <cmath>

namespace PP {
namespace devicetree {

namespace {

class JsonParser {
public:
    explicit JsonParser(const std::string& text) : mText(text) {}

    bool parse(JsonValue* root, std::string* error) {
        mIndex = 0;
        skipWs();
        if (!parseValue(root)) {
            if (error != nullptr) {
                *error = mError.empty() ? "invalid JSON" : mError;
            }
            return false;
        }
        skipWs();
        if (mIndex < mText.size()) {
            if (error != nullptr) {
                *error = "trailing JSON data";
            }
            return false;
        }
        return true;
    }

private:
    const std::string& mText;
    size_t mIndex = 0;
    std::string mError;

    char peek() const { return mIndex < mText.size() ? mText[mIndex] : '\0'; }

    char get() {
        if (mIndex >= mText.size()) {
            return '\0';
        }
        return mText[mIndex++];
    }

    void skipWs() {
        while (mIndex < mText.size() && std::isspace(static_cast<unsigned char>(mText[mIndex]))) {
            ++mIndex;
        }
    }

    bool fail(const char* message) {
        mError = message;
        return false;
    }

    bool parseValue(JsonValue* out) {
        skipWs();
        const char c = peek();
        if (c == '{') {
            return parseObject(out);
        }
        if (c == '[') {
            return parseArray(out);
        }
        if (c == '"') {
            return parseString(out);
        }
        if (c == 't' || c == 'f') {
            return parseBool(out);
        }
        if (c == 'n') {
            return parseNull(out);
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return parseNumber(out);
        }
        return fail("unexpected token");
    }

    bool parseObject(JsonValue* out) {
        if (get() != '{') {
            return fail("expected object");
        }
        out->type = JsonType::Object;
        skipWs();
        if (peek() == '}') {
            get();
            return true;
        }
        for (;;) {
            skipWs();
            if (peek() != '"') {
                return fail("expected object key");
            }
            JsonValue keyValue;
            if (!parseString(&keyValue)) {
                return false;
            }
            skipWs();
            if (get() != ':') {
                return fail("expected colon");
            }
            JsonValue child;
            if (!parseValue(&child)) {
                return false;
            }
            out->objectValue[keyValue.stringValue] = child;
            skipWs();
            const char next = get();
            if (next == '}') {
                return true;
            }
            if (next != ',') {
                return fail("expected comma or end of object");
            }
        }
    }

    bool parseArray(JsonValue* out) {
        if (get() != '[') {
            return fail("expected array");
        }
        out->type = JsonType::Array;
        skipWs();
        if (peek() == ']') {
            get();
            return true;
        }
        for (;;) {
            JsonValue child;
            if (!parseValue(&child)) {
                return false;
            }
            out->arrayValue.push_back(child);
            skipWs();
            const char next = get();
            if (next == ']') {
                return true;
            }
            if (next != ',') {
                return fail("expected comma or end of array");
            }
        }
    }

    bool parseString(JsonValue* out) {
        if (get() != '"') {
            return fail("expected string");
        }
        out->type = JsonType::String;
        while (mIndex < mText.size()) {
            const char c = get();
            if (c == '"') {
                return true;
            }
            if (c == '\\') {
                if (mIndex >= mText.size()) {
                    return fail("bad escape");
                }
                const char esc = get();
                switch (esc) {
                    case '"':
                    case '\\':
                    case '/':
                        out->stringValue += esc;
                        break;
                    case 'b':
                        out->stringValue += '\b';
                        break;
                    case 'f':
                        out->stringValue += '\f';
                        break;
                    case 'n':
                        out->stringValue += '\n';
                        break;
                    case 'r':
                        out->stringValue += '\r';
                        break;
                    case 't':
                        out->stringValue += '\t';
                        break;
                    case 'u':
                        if (mIndex + 4 > mText.size()) {
                            return fail("bad unicode escape");
                        }
                        out->stringValue += '?';
                        mIndex += 4;
                        break;
                    default:
                        return fail("bad escape");
                }
            } else {
                out->stringValue += c;
            }
        }
        return fail("unterminated string");
    }

    bool parseBool(JsonValue* out) {
        if (mText.compare(mIndex, 4, "true") == 0) {
            mIndex += 4;
            out->type = JsonType::Bool;
            out->boolValue = true;
            return true;
        }
        if (mText.compare(mIndex, 5, "false") == 0) {
            mIndex += 5;
            out->type = JsonType::Bool;
            out->boolValue = false;
            return true;
        }
        return fail("bad bool");
    }

    bool parseNull(JsonValue* out) {
        if (mText.compare(mIndex, 4, "null") == 0) {
            mIndex += 4;
            out->type = JsonType::Null;
            return true;
        }
        return fail("bad null");
    }

    bool parseNumber(JsonValue* out) {
        const size_t start = mIndex;
        if (peek() == '-') {
            ++mIndex;
        }
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            ++mIndex;
        }
        if (peek() == '.') {
            ++mIndex;
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                ++mIndex;
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            ++mIndex;
            if (peek() == '+' || peek() == '-') {
                ++mIndex;
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                ++mIndex;
            }
        }
        out->type = JsonType::Number;
        out->numberValue = std::strtod(mText.c_str() + start, nullptr);
        return true;
    }
};

} /* anonymous */

bool jsonParse(const std::string& text, JsonValue* root, std::string* error) {
    if (root == nullptr) {
        if (error != nullptr) {
            *error = "null output";
        }
        return false;
    }
    JsonParser parser(text);
    return parser.parse(root, error);
}

} /* namespace devicetree */
} /* namespace PP */
