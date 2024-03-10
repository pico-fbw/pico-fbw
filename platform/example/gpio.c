/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/gpio.h"

void gpio_setup(u32 pin, PinMode mode) {
    // This function will be called before executing any other GPIO-related functions on a given pin.
    // It should look at the `mode` and configure the `pin` accordingly (see gpio.h for the possible modes).
}

PinState gpio_state(u32 pin) {
    // This function should return the current state of the `pin` as a PinState (either LOW [0] or HIGH [1]).
}

void gpio_set(u32 pin, PinState state) {
    // This function should set the `pin` to the `state` (either LOW [0] or HIGH [1]).
}

void gpio_toggle(u32 pin) {
    // This function should toggle the state of the `pin`.
    // For example, if the pin is currently HIGH, it should be set to LOW, and vice versa.
}
