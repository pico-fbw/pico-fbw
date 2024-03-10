/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/pwm.h"

bool pwm_setup_read(u32 pins[], u32 num_pins) {
    // This function will be called before executing any other PWM READ-related functions on a given pin.
    // It should run any prerequisite setup for reading PWM signals on the given pins.
    // It should return true if the setup was successful, false if not.
}

bool pwm_setup_write(u32 pins[], u32 num_pins, u32 freq) {
    // This function will be called before executing any other PWM WRITE-related functions on a given pin.
    // It should run any prerequisite setup for writing PWM signals on the given pins at the given frequency (in Hz).
    // It should return true if the setup was successful, false if not.
}

float pwm_read_raw(u32 pin) {
    // This function should return the pulsewidth cycle of the PWM signal on the given pin.
    // The duty cycle should be returned in the form of a floating-point number, measured in μs (microseconds).
    // If the pin is invalid or some sort of error occurs, return -1.
}

void pwm_write_raw(u32 pin, u16 duty) {
    // This function should write the given duty cycle to the given pin.
    // The duty cycle is an unsigned 16-bit integer, so it ranges from 0 to 2^16 [65535] (0 being off and 2^16 being on).
}
