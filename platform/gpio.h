#pragma once

#include <stdbool.h>
#include "platform/types.h"

typedef enum PinMode {
    MODE_INPUT,
    MODE_OUTPUT,
    MODE_INPUT_PULLUP,
    MODE_INPUT_PULLDOWN,
} PinMode;

typedef enum PinState {
    STATE_LOW,
    STATE_HIGH,
} PinState;

/**
 * Sets up GPIO on a pin.
 * @param pin GPIO pin number
 * @param mode PinMode to use
 */
void gpio_setup(u32 pin, PinMode mode);

/**
 * @param pin GPIO pin number
 * @return current PinState
 */
PinState gpio_state(u32 pin);

/**
 * @param pin GPIO pin number
 * @param state PinState to set
 */
void gpio_set(u32 pin, PinState state);

/**
 * Toggles the state of a GPIO pin.
 * @param pin GPIO pin number
 */
void gpio_toggle(u32 pin);
