/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "pico/time.h"

#include "platform/time.h"

// Wrapper function to convert the callback signature from `i32 (*)(u32)` to `i64 (*)(alarm_id_t, void*)`for the SDK
static i64 callback_to_sdk(alarm_id_t id, void *data) {
    Callback func = (Callback)data;
    return (i64)func(id) * 1000; // *1000 to convert from ms to us
}

u64 time_us() { return time_us_64(); }

i32 callback_in_ms(u32 ms, Callback callback) { return add_alarm_in_ms(ms, callback_to_sdk, (void *)callback, true); }

bool cancel_callback(u32 id) { return cancel_alarm((alarm_id_t)id); }

void sleep_us_blocking(u64 us) { sleep_us(us); }
