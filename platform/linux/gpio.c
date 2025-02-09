/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <gpiod.h>

#include "platform/gpio.h"

#define GPIOCHIP_DEVICE "gpiochip0"
#define GPIOCHIP_CONSUMER "pico-fbw"
#define MAX_REQUESTS 64

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *requests[MAX_REQUESTS];

void gpio_setup(i16 pin, PinMode mode) {
    // Establish control of the specified gpiochip device
    if (!chip) {
        chip = gpiod_chip_open("/dev/" GPIOCHIP_DEVICE);
        if (!chip) {
            return;
        }
    }
    // Configure settings according to the specified mode
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        return;
    }
    switch (mode) {
        case MODE_INPUT_PULLDOWN:
            if (gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN) != 0) {
                gpiod_line_settings_free(settings);
                return;
            }
            goto INPUT;
        case MODE_INPUT_PULLUP:
            if (gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP) != 0) {
                gpiod_line_settings_free(settings);
                return;
            }
            goto INPUT;
        INPUT:
        case MODE_INPUT:
            if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT) != 0) {
                gpiod_line_settings_free(settings);
                return;
            }
            break;
        case MODE_OUTPUT:
            if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT) != 0) {
                gpiod_line_settings_free(settings);
                return;
            }
            break;
    }
    // Configure a line configuration for the specified pin
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    if (!line_cfg) {
        gpiod_line_settings_free(settings);
        return;
    }
    if (gpiod_line_config_add_line_settings(line_cfg, (unsigned int[]){pin}, 1, settings) != 0) {
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        return;
    }
    gpiod_line_settings_free(settings);
    // Request the specified pin
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    if (!req_cfg) {
        gpiod_line_config_free(line_cfg);
        return;
    }
    gpiod_request_config_set_consumer(req_cfg, GPIOCHIP_CONSUMER);
    struct gpiod_line_request *req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    if (!req) {
        return;
    }
    requests[pin] = req;
}

PinState gpio_state(i16 pin) {
    if (!requests[pin]) {
        return STATE_LOW; // Default fallback
    }
    return gpiod_line_request_get_value(requests[pin], pin) ? STATE_HIGH : STATE_LOW;
}

void gpio_set(i16 pin, PinState state) {
    if (!requests[pin]) {
        return;
    }
    gpiod_line_request_set_value(requests[pin], pin,
                                 state == STATE_HIGH ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
}

void gpio_toggle(i16 pin) {
    gpio_set(pin, gpio_state(pin) == STATE_HIGH ? STATE_LOW : STATE_HIGH);
}
