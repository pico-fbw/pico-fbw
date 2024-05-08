/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/pwm.h"

bool pwm_setup_read(const u32 pins[], u32 num_pins) {
    return true; // Not implemented
}

bool pwm_setup_write(const u32 pins[], u32 num_pins, u32 freq) {
    return true; // Not implemented
}

f32 pwm_read_raw(u32 pin) {
    return 0.f; // Not implemented
}

void pwm_write_raw(u32 pin, f32 pulsewidth) {
    return; // Not implemented
}
