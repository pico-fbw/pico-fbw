/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/pwm.h"

bool pwm_setup_read(u32 pins[], u32 num_pins) {}

void pwm_setup_write(u32 pins[], u32 num_pins, u32 freq) {}

i32 pwm_read_raw(u32 pin) {}

void pwm_write_raw(u32 pin, u16 duty) {}
