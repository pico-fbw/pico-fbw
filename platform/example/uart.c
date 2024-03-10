/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/uart.h"

bool uart_setup(u32 tx, u32 rx, u32 baud) {
    // This function should set up the given TX and RX pins for UART communication at the given baudrate.
    // It should return true if the setup was successful, and false if not.
}

char *uart_read(u32 tx, u32 rx) {
    // This function should read a line from all the given UART pins and return it as a null-terminated string.
    // Memory for this string should be allocated on the heap using malloc(), realloc(), or similar.
    // It will be free()d by the caller.
    // If there is no input currentlym available, this function should return NULL.
}

bool uart_write(u32 tx, u32 rx, const char *str) {
    // This function should write the given string to the specified UART pins.
    // It should return true if the write was successful, and false if not.
    // If your platform has no way to check if the write was successful, just return true.
}
