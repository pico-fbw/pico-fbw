/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "pico/config.h"

#ifdef RASPBERRYPI_PICO_W
    #include <stdio.h>
    #include "pico/cyw43_arch.h"
char buf[1];
#else
    #include "hardware/gpio.h"
#endif

#include "platform/defs.h"

#include "platform/gpio.h"

void gpio_setup(i16 pin, PinMode mode) {
#ifdef RASPBERRYPI_PICO_W
    if (pin >= CYW43_GPIO_OFFSET) {
        return; // Don't setup pins that are mapped to CYW43
    }
#endif
    gpio_init(pin);
    switch (mode) {
        case MODE_INPUT_PULLDOWN:
            gpio_pull_down(pin);
            gpio_set_dir(pin, GPIO_IN);
            goto INPUT;
        case MODE_INPUT_PULLUP:
            gpio_pull_up(pin);
            goto INPUT;
        INPUT:
        case MODE_INPUT:
            gpio_set_dir(pin, GPIO_IN);
            break;
        case MODE_OUTPUT:
            gpio_set_dir(pin, GPIO_OUT);
    }
}

PinState gpio_state(i16 pin) {
#ifdef RASPBERRYPI_PICO_W
    // Pins 0-29 are for regular Pico GPIO, 30-32 will be mapped to CYW43 gpios 0-2
    // This is done so that the same gpio functions can be used but all pins can still be accessed
    return pin >= CYW43_GPIO_OFFSET ? cyw43_arch_gpio_get(pin - CYW43_GPIO_OFFSET) : gpio_get(pin);
#else
    return gpio_get(pin);
#endif
}

void gpio_set(i16 pin, PinState state) {
#ifdef RASPBERRYPI_PICO_W
    if (pin >= CYW43_GPIO_OFFSET) {
        cyw43_arch_gpio_put(pin - CYW43_GPIO_OFFSET, state);
        snprintf(buf, sizeof(buf), (const char *)' '); // Pins act weird without this?! Very confusing
    } else {
        gpio_put(pin, state);
    }
#else
    gpio_put(pin, state);
#endif
}

void gpio_toggle(i16 pin) {
#ifdef RASPBERRYPI_PICO_W
    if (pin >= CYW43_GPIO_OFFSET) {
        cyw43_arch_gpio_put(pin - CYW43_GPIO_OFFSET, !cyw43_arch_gpio_get(pin - CYW43_GPIO_OFFSET));
        snprintf(buf, sizeof(buf), (const char *)' ');
    } else {
        gpio_xor_mask(1u << pin);
    }
#else
    gpio_xor_mask(1u << pin);
#endif
}
