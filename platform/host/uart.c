/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/uart.h"

bool uart_setup(i16 tx, i16 rx, u32 baud) {
    return true; // Not implemented
    (void)tx;
    (void)rx;
    (void)baud;
}

char *uart_read(i16 tx, i16 rx) {
    return NULL; // Not implemented
    (void)tx;
    (void)rx;
}

bool uart_write(i16 tx, i16 rx, const char *str) {
    return true; // Not implemented
    (void)tx;
    (void)rx;
    (void)str;
}
