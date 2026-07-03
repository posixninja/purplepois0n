/*
 * JsonMini.h
 *
 * Minimal JSON parser for ipsw dtree -j output (objects, arrays, strings, numbers).
 */

#ifndef DEVICETREE_JSON_MINI_H_
#define DEVICETREE_JSON_MINI_H_

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace PP {
namespace devicetree {

enum class JsonType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

struct JsonValue {
    JsonType type = JsonType::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;
};

bool jsonParse(const std::string& text, JsonValue* root, std::string* error);

} /* namespace devicetree */
} /* namespace PP */

#endif /* DEVICETREE_JSON_MINI_H_ */
