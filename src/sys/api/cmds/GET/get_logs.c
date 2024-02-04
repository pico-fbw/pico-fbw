/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdio.h>
#include "pico/time.h"
#include "pico/types.h"

#include "sys/log.h"

#include "sys/api/cmds/GET/get_logs.h"

int api_get_logs(const char *cmd, const char *args) {
    uint logCount = log_count();
    if (logCount > 0) {
        printf("{\"logs\":[");
        for (uint i = 0; i < logCount; i++) {
            LogEntry *entry = log_get(i);
            printf("{\"type\":%d,\"msg\":\"%s\",\"code\":%d,\"timestamp\":%llu}",
                   entry->type, entry->msg, entry->code, entry->timestamp);
            if (logCount > 1 && i != logCount - 1) printf(",");
        }
        printf("]}\n");
        return -1;
    }
    return 204;
}
