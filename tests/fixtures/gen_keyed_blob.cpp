/* Generator for tests/fixtures/manifest_db_keyed file BLOB bytes. */
#include <plist/plist.h>
#include <cstdio>
#include <cstdlib>

static plist_t uid(uint64_t value) {
    return plist_new_uid(value);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: gen_keyed_blob OUTPUT.bin\n");
        return 1;
    }

    plist_t objects = plist_new_array();
    plist_array_append_item(objects, plist_new_string("$null"));

    plist_t keys = plist_new_array();
    plist_array_append_item(keys, uid(2));
    plist_array_append_item(keys, uid(3));
    plist_array_append_item(keys, uid(4));

    plist_t vals = plist_new_array();
    plist_array_append_item(vals, uid(5));
    plist_array_append_item(vals, uid(6));
    plist_array_append_item(vals, uid(7));

    plist_t metaDict = plist_new_dict();
    plist_dict_set_item(metaDict, "NS.keys", keys);
    plist_dict_set_item(metaDict, "NS.objects", vals);
    plist_array_append_item(objects, metaDict);

    plist_array_append_item(objects, plist_new_string("Size"));
    plist_array_append_item(objects, plist_new_string("LastModified"));
    plist_array_append_item(objects, plist_new_string("Birth"));
    plist_array_append_item(objects, plist_new_uint(512));
    plist_array_append_item(objects, plist_new_real(1609459200.0));
    plist_array_append_item(objects, plist_new_real(1609455600.0));

    plist_t top = plist_new_dict();
    plist_dict_set_item(top, "root", uid(1));

    plist_t root = plist_new_dict();
    plist_dict_set_item(root, "$version", plist_new_uint(100000));
    plist_dict_set_item(root, "$archiver", plist_new_string("NSKeyedArchiver"));
    plist_dict_set_item(root, "$top", top);
    plist_dict_set_item(root, "$objects", objects);

    char* bin = nullptr;
    uint32_t length = 0;
    plist_to_bin(root, &bin, &length);
    plist_free(root);

    if (!bin || length == 0) {
        return 1;
    }

    FILE* out = std::fopen(argv[1], "wb");
    if (!out) {
        plist_mem_free(bin);
        return 1;
    }
    std::fwrite(bin, 1, length, out);
    std::fclose(out);
    plist_mem_free(bin);
    return 0;
}
