/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/stdio.h"

void stdio_setup() {}

char *stdin_read() {}

int __printflike(1, 2) wrap_printf(const char *fmt, ...) {}
