/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "platform/time.h"

#include "platform/stdio.h"

void stdio_setup() {
    return; // Nothing to do here, the bootloader already sets up stdio
    // See https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-guides/startup.html
}

char *stdin_read() {
    // Very similar to stdin_read() in pico/stdio.c, take a look at that for documentation
    char *buf = NULL;
    u32 i = 0;
    while (true) {
        int c = getchar();
        if (c == '\n' || c == '\r' || c == 0 || c == EOF) {
            break;
        } else {
            buf = try_realloc(buf, (i + 1) * sizeof(char));
            if (!buf)
                return NULL;
            buf[i++] = c;
            sleep_us_blocking(50); // Sleep for a bit to wait for more input (possibly)
        }
    }
    if (i != 0) {
        buf = try_realloc(buf, (i + 1) * sizeof(char));
        if (!buf)
            return NULL;
        buf[i] = '\0';
    }
    return buf;
}

int __printflike(1, 2) wrap_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
    return ret;
}
