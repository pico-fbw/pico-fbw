/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdlib.h>
#include "pico/stdio.h"

#include "hardware/uart.h"

#include "serial.h"

char *stdin_read_line() {
    char *buf = NULL;
    uint i = 0;
    while (true) {
        int c = getchar_timeout_us(STDIO_TIMEOUT_US);
        if (c == '\n' || c == '\r' || c == PICO_ERROR_TIMEOUT) {
            break;
        } else {
            // Otherwise, store character in buffer and increment
            buf = (char*)realloc(buf, (i + 1) * sizeof(char));
            buf[i] = c;
            i++;
        }
    }
    // Null-terminate the buffer if we read a line, otherwise return NULL
    if (i != 0) {
        buf = (char*)realloc(buf, (i + 1) * sizeof(char));
        buf[i] = '\0';
    }
    return buf;
}

char *uart_read_line(uart_inst_t *uart) {
    char *buf = NULL;
    if (uart_is_readable(uart)) {
        uint i = 0;
        while (uart_is_readable_within_us(uart, UART_TIMEOUT_US)) {
            char c = uart_getc(uart);
            if (c == '\r' || c == '\n') {
                break;
            }
            buf = realloc(buf, (i + 1) * sizeof(char));
            buf[i] = c;
            i++;
        }
        // Null terminate the string
        buf = realloc(buf, (i + 1) * sizeof(char));
        buf[i] = '\0';
    }
    return buf;
}
