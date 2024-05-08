/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/uart.h"

bool uart_setup(u32 tx, u32 rx, u32 baud) {
    return true; // Not implemented
}

char *uart_read(u32 tx, u32 rx) {
    return NULL; // Not implemented
}

bool uart_write(u32 tx, u32 rx, const char *str) {
    return true; // Not implemented
}
