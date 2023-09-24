/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
*/

/**
 * Huge thanks to 'markushi' on GitHub for developing the bulk of this servo library! (slightly modified by MylesAndMore)
 * Check that out here: https://github.com/markushi/pico-servo
*/

/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

#include "../sys/config.h"

#include "servo.h"

uint servo_enable(uint gpio_pin) {
    if (config.debug.debug_fbw) printf("[servo] setting up servo on pin %d\n", gpio_pin);
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
    uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    uint freq = config.general.servoHz;
    uint32_t source_hz = clock_get_hz(clk_sys);
    uint32_t div16_top = 16 * source_hz / freq;
    uint32_t top = 1;
    while (true) {
        if (div16_top >= 16 * 5 && div16_top % 5 == 0 && top * 5 <= PWM_TOP_MAX) {
            div16_top /= 5;
            top *= 5;
        } else if (div16_top >= 16 * 3 && div16_top % 3 == 0 && top * 3 <= PWM_TOP_MAX) {
            div16_top /= 3;
            top *= 3;
        } else if (div16_top >= 16 * 2 && top * 2 <= PWM_TOP_MAX) {
            div16_top /= 2;
            top *= 2;
        } else {
            break;
        }
    }
    if (div16_top < 16) {
        if (config.debug.debug_fbw) printf("[servo] ERROR: frequency too large\n");
        return 2;
    } else if (div16_top >= 256 * 16) {
        if (config.debug.debug_fbw) printf("[servo] ERROR: frequency too small\n");
        return 1;
    }
    pwm_hw->slice[slice].div = div16_top;
    pwm_hw->slice[slice].top = top;
    servo_set(gpio_pin, 90); // Set the servo to ~90 (middle) degrees by default
    return 0;
}

void servo_set(uint gpio_pin, uint16_t degree) {
    uint16_t oneMs = PWM_TOP_MAX / 20;
    uint16_t duty_u16 = oneMs + (oneMs * degree) / 180;

    uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    uint8_t channel = pwm_gpio_to_channel(gpio_pin);
    uint32_t top = pwm_hw->slice[slice].top;
    uint32_t cc = duty_u16 * (top + 1) / PWM_TOP_MAX;

    pwm_set_chan_level(slice, channel, cc);
    pwm_set_enabled(slice, true);
}

void servo_disable(uint gpio_pin) {
    uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    pwm_set_enabled(slice, false);
}

void servo_test(uint servos[], uint num_servos, const uint16_t degrees[], const uint num_degrees, const uint pause_between_moves_ms) {
    for (uint8_t d = 0; d < num_degrees; d++) {
        for (uint8_t s = 0; s < num_servos; s++) {
            servo_set(servos[s], degrees[d]);
        }
        sleep_ms(pause_between_moves_ms);
    }
}

void servo_getPins(uint *servos, uint *num_servos) {
    switch (config.general.controlMode) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            servos[0] = config.pins0.servoAil;
            servos[1] = config.pins0.servoElev;
            servos[2] = config.pins0.servoRud;
            *num_servos = 3;
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            servos[0] = config.pins1.servoElevonL;
            servos[1] = config.pins1.servoElevonR;
            *num_servos = 2;
            break;
    }
}
