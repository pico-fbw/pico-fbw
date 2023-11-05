/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
*/

/**
 * This ESC library is a modification of the pico-servo library by 'markushi', thanks for that!
 * Check that out at https://github.com/markushi/pico-servo or in servo.c
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/types.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

#include "flash.h"

#include "esc.h"
#include "servo.h"

uint esc_enable(uint gpio_pin) {
    if (print.fbw) printf("[ESC] setting up ESC on pin %d\n", gpio_pin);
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
    uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    uint freq = (uint)flash.general[GENERAL_ESC_HZ];
    uint32_t source_hz = clock_get_hz(clk_sys);
    uint32_t div16_top = 16 * source_hz / freq;
    uint32_t top = 1;
    while (true) {
        // Try a few small prime factors to get close to the desired frequency.
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
        if (print.fbw) printf("[ESC] ERROR: frequency too large\n");
        return 2;
    } else if (div16_top >= 256 * 16) {
        if (print.fbw) printf("[ESC] ERROR: frequency too small\n");
        return 1;
    }
    pwm_hw->slice[slice].div = div16_top;
    pwm_hw->slice[slice].top = top;
    esc_set(gpio_pin, 0); // Set initial position to 0 to be safe
    return 0;
}

void esc_set(uint gpio_pin, uint16_t speed) {
    // Values have to be between 0 and 100
    // PWM_TOP_MAX = 100% full duty cycle
    uint16_t oneMs = PWM_TOP_MAX / 20;
    uint16_t duty_u16 = oneMs + (oneMs * speed) / 100;

    uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    uint8_t channel = pwm_gpio_to_channel(gpio_pin);
    uint32_t top = pwm_hw->slice[slice].top;
    uint32_t cc = duty_u16 * (top + 1) / PWM_TOP_MAX;

    pwm_set_chan_level(slice, channel, cc);
    pwm_set_enabled(slice, true);
}

void esc_disable(uint gpio_pin) {
    uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    pwm_set_enabled(slice, false);
}
