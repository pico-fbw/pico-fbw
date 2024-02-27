#pragma once

#include <stdbool.h>
#include "platform/int.h"

typedef enum PullDir {
    DOWN,
    UP,
} PullDir;

/**
 * @param pin GPIO pin number
 * @return true if the pin is high, false if the pin is low
*/
bool gpio_on(u32 pin);

/**
 * @param pin GPIO pin number
 * @param on true to set the pin high, false to set the pin low
*/
void gpio_set(u32 pin, bool on);

/**
 * Toggles the state of a GPIO pin.
 * @param pin GPIO pin number
*/
void gpio_toggle(u32 pin);

/**
 * @param pin GPIO pin number
 * @param pull the pull direction to set
*/
void gpio_pull(u32 pin, PullDir pull);
