/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "platform/gpio.h"

void gpio_setup(i16 pin, PinMode mode) {
    return; // Not implemented
    (void)pin;
    (void)mode;
}

PinState gpio_state(i16 pin) {
    return STATE_LOW; // Not implemented
    (void)pin;
}

void gpio_set(i16 pin, PinState state) {
    return; // Not implemented
    (void)pin;
    (void)state;
}

void gpio_toggle(i16 pin) {
    return; // Not implemented
    (void)pin;
}
