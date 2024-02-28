/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdlib.h>
#include <string.h>

#include "platform/stdio.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "platform/uart.h"

// Timeout between waiting for characters in the UART read function (in microseconds)
// This is typically used for GPS modules which operate at a lower baud, so this value must be decently high
#define UART_TIMEOUT_US 4000

/**
 * @param tx the pin number of the TX pin
 * @param rx the pin number of the RX pin
 * @return the UART instance that the pins lie on, or NULL if the pins do not form a valid UART instance
 */
static inline uart_inst_t *uart_inst_from_pins(u32 tx, u32 rx) {
    switch (tx) {
    case 0:
    case 12:
    case 16:
        switch (rx) {
        case 1:
        case 13:
        case 17:
            return uart0;
        default:
            return NULL;
        }
    case 4:
    case 8:
    case 20:
        switch (rx) {
        case 5:
        case 9:
        case 21:
            return uart1;
        default:
            return NULL;
        }
    default:
        return NULL;
    }
}

bool uart_setup(u32 tx, u32 rx, u32 baud) {
    uart_inst_t *uart = uart_inst_from_pins(tx, rx);
    if (!uart)
        return false;
    gpio_set_function(tx, GPIO_FUNC_UART);
    gpio_set_function(rx, GPIO_FUNC_UART);
    gpio_pull_up(tx);
    gpio_pull_up(rx);
    uart_init(uart, baud);
    uart_set_format(uart, 8, 1, UART_PARITY_NONE); // NMEA-0183 format
    // Clear FIFO
    uart_set_fifo_enabled(uart, false);
    irq_set_enabled(uart == uart0 ? UART0_IRQ : UART1_IRQ, true);
    uart_set_fifo_enabled(uart, true);
    return true;
}

char *uart_read(u32 tx, u32 rx) {
    uart_inst_t *uart = uart_inst_from_pins(tx, rx);
    if (!uart)
        return NULL;
    // Very similar to stdin_read() in stdio.c, take a look at that for documentation
    char *buf = NULL;
    if (uart_is_readable(uart)) {
        u32 i = 0;
        while (uart_is_readable_within_us(uart, UART_TIMEOUT_US)) {
            char c = uart_getc(uart);
            if (c == '\r' || c == '\n') {
                break;
            }
            buf = realloc(buf, (i + 1) * sizeof(char));
            if (!buf) {
                free(buf);
                return NULL;
            }
            buf[i++] = c;
        }
        buf = realloc(buf, (i + 1) * sizeof(char));
        if (!buf) {
            free(buf);
            return NULL;
        }
        buf[i] = '\0';
    }
    return buf;
}

bool uart_write(u32 tx, u32 rx, const char *str) {
    uart_inst_t *uart = uart_inst_from_pins(tx, rx);
    if (!uart)
        return false;
    uart_write_blocking(uart, (const u8 *)str, strlen(str) + 1); // +1 to include the null terminator
    return true;
}
