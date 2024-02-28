/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "pico/config.h"

#include "defs.h"

#if defined(RASPBERRYPI_PICO)
#include "hardware/gpio.h"
#elif defined(RASPBERRYPI_PICO_W)
#include <stdio.h>
#include "pico/cyw43_arch.h"
char buf[1];
#else
#error Unsupported Pico variant
#endif

#include "platform/gpio.h"

bool gpio_on(u32 pin) {
#if defined(RASPBERRYPI_PICO)
    return gpio_get(pin);
#elif defined(RASPBERRYPI_PICO_W)
    // Pins 0-29 are for regular Pico GPIO, 30-32 will be mapped to CYW43 gpios 0-2
    // This is done so that the same gpio functions can be used but all pins can still be accessed
    return pin >= CYW43_GPIO_OFFSET ? cyw43_arch_gpio_get(pin - CYW43_GPIO_OFFSET) : gpio_get(pin);
#endif
}

void gpio_set(u32 pin, bool on) {
#if defined(RASPBERRYPI_PICO)
    gpio_put(pin, on);
#elif defined(RASPBERRYPI_PICO_W)
    if (pin >= CYW43_GPIO_OFFSET) {
        cyw43_arch_gpio_put(pin - CYW43_GPIO_OFFSET, on);
        snprintf(buf, sizeof(buf), " "); // Pins act weird without this?! Very confused
    } else {
        gpio_put(pin, on);
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

void gpio_pull(u32 pin, PullDir pull) { gpio_set_pulls(pin, pull, !pull); }
