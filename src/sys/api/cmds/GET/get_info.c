/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "platform/defs.h"
#include "platform/flash.h"

#include "lib/parson.h"
#include "sys/version.h"

#include "get_info.h"

// {"version":"","version_api":"","version_flightplan":"","platform":"","platform_version":"","fs_free":number}

i32 api_get_info(const char *in, char **out) {
    // Calculate fs free space
    struct lfs_fsinfo info;
    lfs_ssize_t blocksUsed = lfs_fs_size(&lfs);
    if (blocksUsed < 0 || lfs_fs_stat(&lfs, &info) < 0) {
        return 500;
    }
    i32 free = (info.block_count - blocksUsed) * info.block_size;
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    json_object_set_string(obj, "version", PICO_FBW_VERSION);
    json_object_set_string(obj, "version_api", PICO_FBW_API_VERSION);
    json_object_set_string(obj, "version_flightplan", FLIGHTPLAN_VERSION);
    json_object_set_string(obj, "platform", PLATFORM_NAME);
    json_object_set_string(obj, "platform_version", PLATFORM_VERSION);
    json_object_set_number(obj, "fs_free", free);
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
    return 200;
    (void)in;
}
