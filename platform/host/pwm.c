/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/pwm.h"

bool pwm_setup_read(const i16 pins[], u32 num_pins) {
    return true; // Not implemented
    (void)pins;
    (void)num_pins;
}

bool pwm_setup_write(const i16 pins[], u32 num_pins, u32 freq) {
    return true; // Not implemented
    (void)pins;
    (void)num_pins;
    (void)freq;
}

f32 pwm_read_raw(i16 pin) {
    return 0.f; // Not implemented
    (void)pin;
}

void pwm_write_raw(i16 pin, f32 pulsewidth) {
    return; // Not implemented
    (void)pin;
    (void)pulsewidth;
}
