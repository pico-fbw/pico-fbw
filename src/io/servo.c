/**
 * Huge thanks to 'markushi' on GitHub for developing the bulk of this servo library! (slightly modified by MylesAndMore)
 * Check them out here: https://github.com/markushi/pico-servo
*/

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

#include "servo.h"
#include "../config.h"

int servo_disable(const uint gpio_pin) {
    const uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    pwm_set_enabled(slice, false);
    return 0;
}

int servo_enable(const uint gpio_pin) {
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
    const uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);

    // Target frequency is 50 hz by default to reflect 20ms PWM signal,
    // subsequent changes of config values can modify this frequency.
    const uint freq = SERVO_HZ;
    uint32_t source_hz = clock_get_hz(clk_sys);

    uint32_t div16_top = 16 * source_hz / freq;
    uint32_t top = 1;
    for (;;) {
        // Try a few small prime factors to get close to the desired frequency.
        if (div16_top >= 16 * 5 && div16_top % 5 == 0 && top * 5 <= SERVO_TOP_MAX) {
            div16_top /= 5;
            top *= 5;
        } else if (div16_top >= 16 * 3 && div16_top % 3 == 0 && top * 3 <= SERVO_TOP_MAX) {
            div16_top /= 3;
            top *= 3;
        } else if (div16_top >= 16 * 2 && top * 2 <= SERVO_TOP_MAX) {
            div16_top /= 2;
            top *= 2;
        } else {
            break;
        }
    }
    if (div16_top < 16) {
        return 2; // freq too large
    } else if (div16_top >= 256 * 16) {
        return 1; // freq too small
    }
    pwm_hw->slice[slice].div = div16_top;
    pwm_hw->slice[slice].top = top;

    return 0;
}

int servo_set(const uint gpio_pin, const uint16_t degree) {
    // values have to be between 0 and 180
    // SERVO_TOP_MAX = 100% full duty cycle

    const uint16_t oneMs = SERVO_TOP_MAX / 20;
    const uint16_t duty_u16 = oneMs + (oneMs * degree) / 180;

    const uint8_t slice = pwm_gpio_to_slice_num(gpio_pin);
    const uint8_t channel = pwm_gpio_to_channel(gpio_pin);

    const uint32_t top = pwm_hw->slice[slice].top;
    const uint32_t cc = duty_u16 * (top + 1) / SERVO_TOP_MAX;

    pwm_set_chan_level(slice, channel, cc);
    pwm_set_enabled(slice, true);

    return true;
}