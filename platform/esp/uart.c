/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdlib.h>
#include <string.h>
#include "driver/uart.h" // https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/peripherals/uart.html
#include "driver/gpio.h"

#include "platform/stdio.h"

#include "platform/uart.h"

#define UART_RX_BUFFER_SIZE 1024
#define UART_TX_BUFFER_SIZE UART_RX_BUFFER_SIZE
#define UART_PORT_START UART_NUM_1 // UART_NUM_0 is used by the console; on most devboards it is conenected to a USB bridge

typedef struct UARTInstance {
    u32 tx, rx;
    uart_port_t port;
} UARTInstance;

static UARTInstance instances[UART_NUM_MAX];

/**
 * @return a pointer to the UART instance that uses the given pins, or NULL if no such instance exists
 */
static UARTInstance *uart_instance_from_pins(u32 tx, u32 rx) {
    for (uart_port_t port = UART_PORT_START; port < UART_NUM_MAX; port++) {
        if (instances[port].tx == tx && instances[port].rx == rx)
            return &instances[port];
    }
    return NULL;
}

bool uart_setup(u32 tx, u32 rx, u32 baud) {
    for (uart_port_t port = UART_PORT_START; port < UART_NUM_MAX; port++) {
        // Find an available UART port
        if (!uart_is_driver_installed(port)) {
            // Install the driver onto the port and configure it
            if (uart_driver_install(port, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE, 0, NULL, 0) != ESP_OK)
                return false;
            instances[port] = (UARTInstance){.tx = tx, .rx = rx, .port = port};
            gpio_pullup_en(tx);
            gpio_pullup_en(rx);
            const uart_config_t config = {
                .baud_rate = baud,
                .data_bits = UART_DATA_8_BITS,
                .parity = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                .source_clk = UART_SCLK_DEFAULT,
            };
            if (uart_param_config(port, &config) != ESP_OK)
                return false;
            if (uart_set_pin(port, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK)
                return false;
            return uart_flush_input(port) == ESP_OK;
        }
    }
    return false; // No available UART ports
}

char *uart_read(u32 tx, u32 rx) {
    UARTInstance *instance = uart_instance_from_pins(tx, rx);
    if (!instance)
        return NULL;
    // Very similar to stdin_read() in pico/stdio.c, take a look at that for documentation
    char *buf = NULL;
    u32 i = 0;
    while (i < UART_RX_BUFFER_SIZE - 1) {
        char c;
        if (uart_read_bytes(instance->port, (void *)&c, 1, 0) <= 0)
            break;
        if (c == '\n' || c == '\r')
            break;
        else {
            buf = try_realloc(buf, (i + 1) * sizeof(char));
            if (!buf)
                return NULL;
            buf[i++] = c;
        }
    }
    if (i != 0) {
        buf = try_realloc(buf, (i + 1) * sizeof(char));
        if (!buf)
            return NULL;
        buf[i] = '\0';
    }
    return buf;
}

bool uart_write(u32 tx, u32 rx, const char *str) {
    UARTInstance *instance = uart_instance_from_pins(tx, rx);
    if (!instance)
        return false;
    return uart_write_bytes(instance->port, (const void *)str, strlen(str)) == ESP_OK;
}
