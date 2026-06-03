/*
 * KeyedArchiverPlist.cpp
 */

#include "KeyedArchiverPlist.h"
#include <plist/plist.h>

namespace PP {

namespace {

uint64_t plistNodeUInt64(plist_t node) {
    if (!node) {
        return 0;
    }
    switch (plist_get_node_type(node)) {
        case PLIST_UINT: {
            uint64_t value = 0;
            plist_get_uint_val(node, &value);
            return value;
        }
        case PLIST_REAL: {
            double value = 0.0;
            plist_get_real_val(node, &value);
            return static_cast<uint64_t>(value);
        }
        case PLIST_DATE: {
            int64_t sec = 0;
            plist_get_unix_date_val(node, &sec);
            return static_cast<uint64_t>(sec);
        }
        default:
            return 0;
    }
}

uint64_t plistDictUInt64(plist_t dict, const char* key) {
    return plistNodeUInt64(plist_dict_get_item(dict, key));
}

uint64_t plistDictDateSeconds(plist_t dict, const char* key) {
    plist_t node = plist_dict_get_item(dict, key);
    return plistNodeUInt64(node);
}

std::string plistNodeString(plist_t node) {
    if (!node || plist_get_node_type(node) != PLIST_STRING) {
        return "";
    }
    char* value = nullptr;
    plist_get_string_val(node, &value);
    if (!value) {
        return "";
    }
    std::string result(value);
    free(value);
    return result;
}

bool plistNodeHasData(plist_t node) {
    if (!node || plist_get_node_type(node) != PLIST_DATA) {
        return false;
    }
    char* bytes = nullptr;
    uint64_t length = 0;
    plist_get_data_val(node, &bytes, &length);
    if (bytes) {
        free(bytes);
    }
    return length > 0;
}

plist_t resolveObject(plist_t objects, plist_t node) {
    if (!node) {
        return node;
    }
    if (plist_get_node_type(node) == PLIST_UID) {
        uint64_t uid = 0;
        plist_get_uid_val(node, &uid);
        if (objects) {
            return plist_array_get_item(objects, uid);
        }
    }
    return node;
}

void applyMetadataFields(plist_t dict, KeyedArchiverFileMetadata& out) {
    out.valid = true;
    out.size = plistDictUInt64(dict, "Size");
    out.mtime = plistDictDateSeconds(dict, "LastModified");
    out.ctime = plistDictDateSeconds(dict, "Birth");

    plist_t encNode = plist_dict_get_item(dict, "EncryptionKey");
    if (encNode) {
        out.hasEncryptionKey = plistNodeHasData(encNode) ||
                               !plistNodeString(encNode).empty();
    }
}

KeyedArchiverFileMetadata scanKeyedObjects(plist_t objects) {
    if (!objects || plist_get_node_type(objects) != PLIST_ARRAY) {
        return KeyedArchiverFileMetadata();
    }

    const uint32_t size = plist_array_get_size(objects);
    for (uint32_t i = 0; i < size; ++i) {
        plist_t item = plist_array_get_item(objects, i);
        if (!item || plist_get_node_type(item) != PLIST_DICT) {
            continue;
        }

        plist_t keysNode = plist_dict_get_item(item, "NS.keys");
        plist_t valsNode = plist_dict_get_item(item, "NS.objects");
        if (!keysNode || !valsNode) {
            KeyedArchiverFileMetadata direct;
            applyMetadataFields(item, direct);
            if (direct.valid && direct.size > 0) {
                return direct;
            }
            continue;
        }

        KeyedArchiverFileMetadata resolved;
        const uint32_t count = plist_array_get_size(keysNode);
        for (uint32_t k = 0; k < count; ++k) {
            plist_t keyNode = resolveObject(objects, plist_array_get_item(keysNode, k));
            plist_t valNode = resolveObject(objects, plist_array_get_item(valsNode, k));
            const std::string key = plistNodeString(keyNode);
            if (key == "Size") {
                resolved.size = plistNodeUInt64(valNode);
                resolved.valid = true;
            } else if (key == "LastModified") {
                resolved.mtime = plistNodeUInt64(valNode);
            } else if (key == "Birth") {
                resolved.ctime = plistNodeUInt64(valNode);
            } else if (key == "EncryptionKey") {
                resolved.hasEncryptionKey = plistNodeHasData(valNode) ||
                                          !plistNodeString(valNode).empty();
            }
        }
        if (resolved.valid) {
            return resolved;
        }
    }

    return KeyedArchiverFileMetadata();
}

} /* anonymous namespace */

KeyedArchiverFileMetadata KeyedArchiverPlist::parsePlainDict(plist_t root) {
    KeyedArchiverFileMetadata out;
    applyMetadataFields(root, out);
    return out;
}

KeyedArchiverFileMetadata KeyedArchiverPlist::parseKeyedArchiver(plist_t root) {
    plist_t objects = plist_dict_get_item(root, "$objects");
    if (!objects) {
        return KeyedArchiverFileMetadata();
    }
    return scanKeyedObjects(objects);
}

KeyedArchiverFileMetadata KeyedArchiverPlist::parseFileMetadata(const std::vector<uint8_t>& blob) {
    if (blob.empty()) {
        return KeyedArchiverFileMetadata();
    }

    plist_t root = nullptr;
    const plist_err_t err = plist_from_memory(reinterpret_cast<const char*>(blob.data()),
                                            static_cast<uint32_t>(blob.size()),
                                            &root, nullptr);
    if (err != PLIST_ERR_SUCCESS || root == nullptr) {
        return KeyedArchiverFileMetadata();
    }

    KeyedArchiverFileMetadata out;
    if (plist_get_node_type(root) == PLIST_DICT) {
        plist_t archiver = plist_dict_get_item(root, "$archiver");
        if (archiver && plistNodeString(archiver) == "NSKeyedArchiver") {
            out = parseKeyedArchiver(root);
        } else {
            out = parsePlainDict(root);
        }
    }

    plist_free(root);
    return out;
}

} /* namespace PP */
