#pragma once

#include <stdbool.h>
#include "platform/int.h"

/**
 * Sets up `pins[]` for reading PWM signals.
 * @param pins array of pins to setup for reading PWM signals
 * @param num_pins number of pins in `pins[]`
 * @return true if all pins were successfully set up
 * @note This is a low-level platform function, you are probably looking to use `receiver_enable()` instead.
 */
bool pwm_setup_read(u32 pins[], u32 num_pins);

/**
 * Sets up `pins[]` for writing PWM signals.
 * @param pins array of pins to setup for writing PWM signals
 * @param num_pins number of pins in `pins[]`
 * @param freq frequency of PWM signal in hz, typically 50
 * @note This is a low-level platform function, you are probably looking to use something like `servo_enable()` or
 * `esc_enable()` instead.
 */
void pwm_setup_write(u32 pins[], u32 num_pins, u32 freq);

/**
 * Reads the PWM signal on `pin`.
 * @param pin pin to read PWM signal from
 * @return the pulsewidth of the PWM signal in ___, or -1 if the pin is invalid
 * @note `pin` must have been previously set up to read PWM signals using `pwm_setup_read()`
 * @note This is a low-level platform function, you are probably looking to use `receiver_get()` instead.
 */
i32 pwm_read_raw(u32 pin);

// TODO: what is the pulsweidth measured in? ^

/**
 * Writes a PWM signal to `pin`.
 * @param pin pin to write PWM signal to
 * @param value the value to write to the PWM signal (0-255)
 * @note `pin` must have been previously set up to write PWM signals using `pwm_setup_write()`
 * @note This is a low-level platform function, you are probably looking to use `servo_set()` or `esc_set()` instead.
 */
void pwm_write_raw(u32 pin, u16 duty);
