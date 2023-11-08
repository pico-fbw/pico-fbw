/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdlib.h>
#include "pico/stdio.h"

#include "hardware/uart.h"

#include "serial.h"

static inline char *tryRealloc(char *buf, size_t size) {
    char *nbuf = (char*)realloc(buf, size);
    if (!nbuf) {
        free(buf);
    }
    return nbuf;
}

char *stdin_read_line() {
    char *buf = NULL;
    uint i = 0;
    while (true) {
        int c = getchar_timeout_us(STDIO_TIMEOUT_US);
        if (c == '\n' || c == '\r' || c == PICO_ERROR_TIMEOUT) {
            break;
        } else {
            // Otherwise, store character in buffer and increment
            buf = tryRealloc(buf, (i + 1) * sizeof(char));
            if (!buf) return NULL;
            buf[i++] = c;
        }
    }
    // Null-terminate the buffer if we read a line, otherwise return NULL
    if (i != 0) {
        buf = tryRealloc(buf, (i + 1) * sizeof(char));
        if (!buf) return NULL;
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
            buf = tryRealloc(buf, (i + 1) * sizeof(char));
            if (!buf) return NULL;
            buf[i++] = c;
        }
        // Null terminate the string
        buf = tryRealloc(buf, (i + 1) * sizeof(char));
        if (!buf) return NULL;
        buf[i] = '\0';
    }
    return buf;
}
