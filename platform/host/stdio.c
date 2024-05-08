/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/stdio.h"

void stdio_setup() {
    return;
}

char *stdin_read() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, stdin);
    if (read == -1) {
        free(line);
        return NULL;
    }
    line[strcspn(line, "\n")] = 0; // Remove trailing newline
    return line;
}

int __printflike(1, 2) wrap_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}
