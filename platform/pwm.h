#pragma once

#include <stdbool.h>
#include "platform/int.h"

/**
 * Sets up `pins[]` for reading PWM signals.
 * @param pins array of pins to setup for reading PWM signals
 * @param num_pins number of pins in `pins[]`
 * @return true if all pins were successfully set up
 */
bool pwm_setup_read(u32 pins[], u32 num_pins);

/**
 * Sets up `pins[]` for writing PWM signals.
 * @param pins array of pins to setup for writing PWM signals
 * @param num_pins number of pins in `pins[]`
 * @param freq frequency of PWM signal in hz, typically 50
 * @return true if all pins were successfully set up
 */
bool pwm_setup_write(u32 pins[], u32 num_pins, u32 freq);

/**
 * Reads the PWM signal on `pin`.
 * @param pin pin to read PWM signal from
 * @return the duty cycle of the PWM signal (0-2^16), or -1 if the pin is invalid
 * @note The duty cycle is actually returned as a `u16` but is cast to `i32` to allow for the -1 return value
 * @note `pin` must have been previously set up to read PWM signals using `pwm_setup_read()`
 */
i32 pwm_read_raw(u32 pin);

/**
 * Writes a PWM signal to `pin`.
 * @param pin pin to write PWM signal to
 * @param value the duty cycle to write to the PWM signal (0-2^16)
 * @note `pin` must have been previously set up to write PWM signals using `pwm_setup_write()`
 */
void pwm_write_raw(u32 pin, u16 duty);
