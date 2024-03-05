/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "driver/gpio.h" // https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/peripherals/gpio.html

#include "platform/gpio.h"

void gpio_setup(u32 pin, PinMode mode) {
    const gpio_config_t config = {
        .pin_bit_mask = 1u << pin,
        .mode = mode == OUTPUT ? GPIO_MODE_INPUT_OUTPUT : GPIO_MODE_INPUT, // OUTPUT is mapped to INPUT_OUTPUT so toggle works
        .pull_up_en = mode == INPUT_PULLUP ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = mode == INPUT_PULLDOWN ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&config);
}

PinState gpio_state(u32 pin) {
    return gpio_get_level((gpio_num_t)pin);
}

void gpio_set(u32 pin, PinState state) {
    gpio_set_level((gpio_num_t)pin, (u32)state);
}

void gpio_toggle(u32 pin) {
    gpio_set_level((gpio_num_t)pin, !gpio_get_level((gpio_num_t)pin));
}
