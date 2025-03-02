/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/stdio.h"

void stdio_setup() {
    // This function will be called before executing any other stdio-related functions.
    // It should configure all relevant stdio sources for input and output (e.g. UART, USB, etc.).
    // If your platform doesn't need to explicitly set up any stdio sources, you can leave this function empty.
}

char *stdin_read() {
    // This function should read ONE line from all stdin sources and return it as a null-terminated string.
    // Any trailing characters (newline, carriage return, etc.) should be removed.
    // Memory for this string should be allocated on the heap using malloc(), realloc(), or similar.
    // It will be free()d by the caller.
    // If there is no input currently available, this function should return NULL.
}
