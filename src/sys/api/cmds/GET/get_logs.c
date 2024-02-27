/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include "sys/log.h"
#include "sys/print.h"

#include "get_logs.h"

i32 api_get_logs(const char *cmd, const char *args) {
    u32 logCount = log_count();
    if (logCount > 0) {
        printraw("{\"logs\":[");
        for (u32 i = 0; i < logCount; i++) {
            LogEntry *entry = log_get(i);
            printraw("{\"type\":%d,\"msg\":\"%s\",\"code\":%d,\"timestamp\":%llu}",
                   entry->type, entry->msg, entry->code, entry->timestamp);
            if (logCount > 1 && i != logCount - 1) printraw(",");
        }
        printraw("]}\n");
        return -1;
    }
    return 204;
}
