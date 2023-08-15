/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Markus Hintersteiner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * **/

/**
 * This ESC library is a modification of the pico-servo library by 'markushi', thanks for that!
 * Check that out at https://github.com/markushi/pico-servo or in servo.c
*/

/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

#include "../config.h"

#include "esc.h"
#include "servo.h"

uint esc_enable(const uint gpio_pin) {
    FBW_DEBUG_printf("[ESC] setting up ESC on pin %d\n", gpio_pin);
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
    const uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    const uint freq = SERVO_HZ;
    uint32_t source_hz = clock_get_hz(clk_sys);
    uint32_t div16_top = 16 * source_hz / freq;
    uint32_t top = 1;
    for (;;) {
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
        FBW_DEBUG_printf("[ESC] ERROR: frequency too large\n");
        return 2;
    } else if (div16_top >= 256 * 16) {
        FBW_DEBUG_printf("[ESC] ERROR: frequency too small\n");
        return 1;
    }
    pwm_hw->slice[slice].div = div16_top;
    pwm_hw->slice[slice].top = top;
    return 0;
}

void esc_disable(const uint gpio_pin) {
    const uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    pwm_set_enabled(slice, false);
}

void esc_set(const uint gpio_pin, const uint16_t degree) {
    // Values have to be between 0 and 100
    // PWM_TOP_MAX = 100% full duty cycle
    const uint16_t oneMs = PWM_TOP_MAX / 20;
    const uint16_t duty_u16 = oneMs + (oneMs * degree) / 100;

    const uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    const uint8_t channel = pwm_gpio_to_channel(gpio_pin);
    const uint32_t top = pwm_hw->slice[slice].top;
    const uint32_t cc = duty_u16 * (top + 1) / PWM_CH0_CC_B_LSB;

    pwm_set_chan_level(slice, channel, cc);
    pwm_set_enabled(slice, true);
}
