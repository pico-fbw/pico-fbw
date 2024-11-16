/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "lib/parson.h"

#include "sys/log.h"
#include "sys/print.h"

#include "get_logs.h"

i32 api_handle_get_logs(const char *in, char **out) {
    u32 logCount = log_count();
    if (logCount == 0)
        return 204;
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    JSON_Value *logs = json_value_init_array();
    JSON_Array *arr = json_value_get_array(logs);
    for (u32 i = 0; i < logCount; i++) {
        LogEntry *entry = log_get(i);
        JSON_Value *log = json_value_init_object();
        JSON_Object *logObj = json_value_get_object(log);
        json_object_set_number(logObj, "type", entry->type);
        json_object_set_string(logObj, "msg", entry->msg);
        json_object_set_number(logObj, "code", entry->code);
        json_object_set_number(logObj, "timestamp", entry->timestamp);
        json_array_append_value(arr, log);
    }
    json_object_set_value(obj, "logs", logs);
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
    return 200;
    (void)in;
}

// {"logs":[{"type":number,"msg":"","code":number,"timestamp":number}]}

i32 api_get_logs(const char *args) {
    char *out = NULL;
    i32 res = api_handle_get_logs(args, &out);
    if (res != 200) {
        if (out)
            json_free_serialized_string(out);
        return res;
    }
    printraw("%s\n", out);
    json_free_serialized_string(out);
    return -1;
}
