/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "pico/config.h"

#include "platform/defs.h"

#if defined(RASPBERRYPI_PICO)
    #include "hardware/gpio.h"
#elif defined(RASPBERRYPI_PICO_W)
    #include <stdio.h>
    #include "pico/cyw43_arch.h"
char buf[1];
#else
    #error "Unsupported Pico variant"
#endif

#include "platform/gpio.h"

void gpio_setup(u32 pin, PinMode mode) {
    gpio_init(pin);
    switch (mode) {
        case INPUT_PULLDOWN:
            gpio_pull_down(pin);
            gpio_set_dir(pin, GPIO_IN);
            break;
        case INPUT_PULLUP:
            gpio_pull_up(pin);
        /* fall through */
        case INPUT:
            gpio_set_dir(pin, GPIO_IN);
            break;
        case OUTPUT:
            // TODO: should this be both in and out?
            gpio_set_dir(pin, GPIO_OUT);
    }
}

PinState gpio_state(u32 pin) {
#if defined(RASPBERRYPI_PICO)
    return gpio_get(pin);
#elif defined(RASPBERRYPI_PICO_W)
    // Pins 0-29 are for regular Pico GPIO, 30-32 will be mapped to CYW43 gpios 0-2
    // This is done so that the same gpio functions can be used but all pins can still be accessed
    return pin >= CYW43_GPIO_OFFSET ? cyw43_arch_gpio_get(pin - CYW43_GPIO_OFFSET) : gpio_get(pin);
#endif
}

void gpio_set(u32 pin, PinState state) {
#if defined(RASPBERRYPI_PICO)
    gpio_put(pin, state);
#elif defined(RASPBERRYPI_PICO_W)
    if (pin >= CYW43_GPIO_OFFSET) {
        cyw43_arch_gpio_put(pin - CYW43_GPIO_OFFSET, state);
        snprintf(buf, sizeof(buf), " "); // Pins act weird without this?! Very confused
    } else {
        gpio_put(pin, state);
    }
#endif
}

void gpio_toggle(u32 pin) {
#if defined(RASPBERRYPI_PICO)
    gpio_xor_mask(1u << pin);
#elif defined(RASPBERRYPI_PICO_W)
    if (pin >= CYW43_GPIO_OFFSET) {
        cyw43_arch_gpio_put(pin - CYW43_GPIO_OFFSET, !cyw43_arch_gpio_get(pin - CYW43_GPIO_OFFSET));
        snprintf(buf, sizeof(buf), " ");
    } else {
        gpio_xor_mask(1u << pin);
    }
#endif
}
