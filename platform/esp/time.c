/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/time.h"

// https://docs.espressif.com/projects/esp-idf/en/v4.2.1/esp32/api-reference/peripherals/timer.html

u64 time_us() {}

i32 callback_in_ms(u32 ms, Callback callback) {}

bool cancel_callback(u32 id) {}

void sleep_us_blocking(u64 us) {}
