/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/gpio.h"

void gpio_setup(u32 pin, PinMode mode) {
    return; // Not implemented
}

PinState gpio_state(u32 pin) {
    return LOW; // Not implemented
}

void gpio_set(u32 pin, PinState state) {
    return; // Not implemented
}

void gpio_toggle(u32 pin) {
    return; // Not implemented
}
