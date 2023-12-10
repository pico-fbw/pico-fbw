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
#include <stdio.h>
#include <stdlib.h>
#include "pico/time.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

#include "display.h"
#include "flash.h"
#include "platform.h"
#include "pwm.h"

#include "../sys/log.h"

#include "esc.h"
#include "servo.h"

/**
 * Waits up to timeout_ms for the throttle input to move, then wait for duration_ms after it stops moving, and write to *detent.
 * @param gpio_pin the GPIO pin the ESC is attached to
 * @param detent the detent to write to
 * @param timeout_ms the timeout (before the throttle is moved) in milliseconds
 * @param duration_ms the duration (after the throttle stops moving) in milliseconds
 * @return Whether a timeout occured.
*/
static bool waitForDetent(uint gpio_pin, float *detent, uint32_t timeout_ms, uint32_t duration_ms) {
    absolute_time_t wait = make_timeout_time_ms(timeout_ms);
    uint16_t lastReading = pwm_read((uint)flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC);
    bool hasMoved = (abs(((uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC) - lastReading)) > flash.control[CONTROL_DEADBAND]);
    while (!hasMoved && !time_reached(wait)) {
        hasMoved = (abs(((uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC) - lastReading)) > flash.control[CONTROL_DEADBAND]);
    }
    if (time_reached(wait)) {
        if (print.fbw) printf("[ESC] ESC calibration timed out!\n");
        return false;
    }

    while (true) {
        esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC));
        hasMoved = (abs(((uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC) - lastReading)) > flash.control[CONTROL_DEADBAND]);
        if (!hasMoved) {
            wait = make_timeout_time_ms(duration_ms);
            while (!hasMoved && !time_reached(wait)) {
                hasMoved = (abs(((uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC) - lastReading)) > flash.control[CONTROL_DEADBAND]);
            }
            if (time_reached(wait)) break;
        }
        lastReading = pwm_read((uint)flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC);
    }
    *detent = (float)lastReading;
    esc_set((uint)flash.pins[PINS_ESC_THROTTLE], 0);
}

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

bool esc_calibrate(uint gpio_pin) {
    log_message(INFO, "Calibrating ESC", 200, 0, false);
    char pBar[DISPLAY_MAX_LINE_LEN] = { [0 ... DISPLAY_MAX_LINE_LEN - 1] = ' '};
    if (platform_is_fbw()) {
        display_pBarStr(pBar, 0);
        display_text("Select idle", "thrust.", "", pBar, true);
    }
    if (!waitForDetent(gpio_pin, &flash.control[CONTROL_THROTTLE_DETENT_IDLE], 10000, 4000)) return false;
    if (platform_is_fbw()) {
        display_pBarStr(pBar, 33);
        display_text("Select max", "continuous", "thrust (MCT).", pBar, true);
    }
    if (!waitForDetent(gpio_pin, &flash.control[CONTROL_THROTTLE_DETENT_MCT], 10000, 2000)) return false;
    if (platform_is_fbw()) {
        display_pBarStr(pBar, 66);
        display_text("Select max", "thrust.", "", pBar, true);
    }
    if (!waitForDetent(gpio_pin, &flash.control[CONTROL_THROTTLE_DETENT_MAX], 10000, 1000)) return false;
    if (print.fbw) printf("[ESC] final detents: %d, %d, %d\n", (uint16_t)flash.control[CONTROL_THROTTLE_DETENT_IDLE],
                          (uint16_t)flash.control[CONTROL_THROTTLE_DETENT_MCT], (uint16_t)flash.control[CONTROL_THROTTLE_DETENT_MAX]);
    flash.control[CONTROL_THROTTLE_DETENTS_CALIBRATED] = true;
    if (print.fbw) printf("[ESC] saving detents to flash\n");
    flash_save();
    log_clear(INFO);
    return true;
}

bool esc_isCalibrated() {
    return (bool)flash.control[CONTROL_THROTTLE_DETENTS_CALIBRATED];
}

void esc_disable(uint gpio_pin) {
    uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    pwm_set_enabled(slice, false);
}
