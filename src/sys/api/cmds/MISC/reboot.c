/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdbool.h>
#include "platform/sys.h"

#include "lib/parson.h"

#include "reboot.h"

// {"bootloader":boolean}

i32 api_reboot(const char *args) {
    JSON_Value *root = json_parse_string(args);
    if (!root)
        return 400;
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return 400;
    }
    JSON_Value *bootloader = json_object_get_value(obj, "bootloader");
    if (!bootloader || json_value_get_type(bootloader) != JSONBoolean) {
        json_value_free(root);
        return 400;
    }
    sys_reboot((bool)json_value_get_boolean(bootloader));
    json_value_free(bootloader);
    json_value_free(root);
    return -1;
}
