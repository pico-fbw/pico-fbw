/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

// TODO: implement PWM using MCPWM
// (https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/mcpwm.html)

#include "esp_err.h"

#include "platform/pwm.h"

#define NUM_PWM_CHANNELS 8

typedef struct PWMChannel {
    u32 pin;
    bool active;
    // Only used for PWM input (MCPWM)
    // ...
    // Only used for PWM output (LEDC)
    // ...
} PWMChannel;

static PWMChannel channels[NUM_PWM_CHANNELS];

/**
 * @param index if not NULL, the index of the returned channel will be written here
 * @return a pointer to the first available PWM output channel, or NULL if none are available
 */
static PWMChannel *get_available_channel(u32 *index) {
    for (size_t i = 0; i < count_of(channels); i++) {
        if (!channels[i].active) {
            channels[i].active = true;
            if (index)
                *index = i;
            return &channels[i];
        }
    }
    return NULL;
}

/**
 * @return a pointer to the PWM output channel for the given pin, or NULL if none match
 */
static PWMChannel *get_channel(u32 pin) {
    for (size_t i = 0; i < count_of(channels); i++) {
        if (channels[i].pin == pin)
            return &channels[i];
    }
    return NULL;
}

bool pwm_setup_read(u32 pins[], u32 num_pins) {
    return true;
}

bool pwm_setup_write(u32 pins[], u32 num_pins, u32 freq) {
    return true;
}

i32 pwm_read_raw(u32 pin) {
    PWMChannel *channel = get_channel(pin);
    if (!channel)
        return -1;
    return 0;
}

void pwm_write_raw(u32 pin, u16 duty) {
    PWMChannel *channel = get_channel(pin);
    if (!channel)
        return;
}
