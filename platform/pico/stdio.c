/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdio.h"

#include "platform/stdio.h"

// Timeout between waiting for characters in the stdio read function (in microseconds)
#define STDIO_TIMEOUT_US 1000

void stdio_setup() {
    stdio_init_all(); // The stdio types that are initializes here depend on what gets defined in platform/pico/CMakeLists.txt
}

char *stdin_read() {
    char *buf = NULL;
    u32 i = 0;
    while (true) {
        // Try to get a character from stdin before the timeout
        i32 c = getchar_timeout_us(STDIO_TIMEOUT_US);
        if (c == '\n' || c == '\r' || c == PICO_ERROR_TIMEOUT) {
            // Either timed out or met the end of a line, end
            break;
        } else {
            // Recieved a valid character, resize the buffer and store it
            buf = realloc(buf, (i + 1) * sizeof(char));
            if (!buf) {
                free(buf);
                return NULL;
            }
            buf[i++] = c;
        }
    }
    // Done reading, null-terminate the buffer if we read a line
    if (i != 0) {
        buf = realloc(buf, (i + 1) * sizeof(char));
        if (!buf) {
            free(buf);
            return NULL; // Nothing was read
        }
        buf[i] = '\0';
    }
    return buf;
}

int __printflike(1, 2) wrap_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}
