/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/time.h"

#include "platform/time.h"

// Wrapper function to convert the callback signature from `i32 (*)()` to `i64 (*)(alarm_id_t, void*)` for the SDK
static i64 callback_to_sdk(alarm_id_t id, void *udata) {
    CallbackData *data = (CallbackData *)udata;
    if (!data)
        return 0;
    i32 reschedule = data->callback();
    if (reschedule == 0)
        free(data);                  // Free the data if the callback is not rescheduled
    return (i64)(reschedule * 1000); // Convert to microseconds
    (void)id;                        // Unused
}

u64 time_us() {
    return time_us_64();
}

CallbackData *callback_in_ms(u32 ms, Callback callback) {
    CallbackData *data = malloc(sizeof(CallbackData));
    if (!data)
        return NULL;
    data->callback = callback;
    data->id = add_alarm_in_ms(ms, callback_to_sdk, (void *)data, true);
    if (data->id < 0) {
        free(data);
        return NULL;
    }
    return data;
}

void cancel_callback(CallbackData *data) {
    if (data) {
        cancel_alarm(data->id);
        free(data);
    }
}

void sleep_us_blocking(u64 us) {
    sleep_us(us);
}
