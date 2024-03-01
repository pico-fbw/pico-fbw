/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/uart.h"

// https://docs.espressif.com/projects/esp-idf/en/v4.2.1/esp32/api-reference/peripherals/uart.html

bool uart_setup(u32 tx, u32 rx, u32 baud) {}

char *uart_read(u32 tx, u32 rx) {}

bool uart_write(u32 tx, u32 rx, const char *str) {}
