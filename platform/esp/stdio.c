/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform/stdio.h"

void stdio_setup() {
    // TODO: are both usb and uart initialized as stdout sources automatically (see esp_vfs_console_register,
    // esp_vfs_dev_uart_use_driver) ?
}

char *stdin_read() {
    char *buf = NULL;
    u32 i = 0;
    while (true) {
        int c = getchar();
        if (c == '\n' || c == '\r' || c == 0 || c == EOF) {
            // Either timed out or met the end of a line, end
            break;
        } else {
            // Recieved a valid character, resize the buffer and store it
            buf = try_realloc(buf, (i + 1) * sizeof(char));
            if (!buf)
                return NULL;
            buf[i++] = c;
        }
    }
    // Done reading, null-terminate the buffer if we read a line
    if (i != 0) {
        buf = try_realloc(buf, (i + 1) * sizeof(char));
        if (!buf)
            return NULL; // Nothing was read
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
