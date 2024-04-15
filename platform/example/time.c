/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/time.h"

// An important part of the timekeeping system are callbacks. These are functions that are scheduled to be called at a certain
// time in the future. As a part of the CallbackData that you see here, there is a callback ID (__callback_id_t). You need to
// define what this type is in defs.h.

u64 time_us() {
    // This function should return the amount of time since the system powered on, in microseconds.
}

CallbackData *callback_in_ms(u32 ms, Callback callback) {
    // This function is tricky! Take some time on it, and take a look at the other platforms for inspiration.

    // This function should schedule a callback to be called in `ms` milliseconds.

    // The Callback will be a function with the signature `i32 (*)()`. This means it will return an `i32`.
    // The return value of the Callback is significant. If zero, the callback should not be rescheduled.
    // If positive, the callback should be rescheduled in that many milliseconds.
    // Most platforms have an SDK function that is able to take a pointer to user data.
    // You can have a singular function that is always called by the SDK, and then
    // have it call the actual callback function through the user data (platforms like the pico and esp do this).

    // This function also needs to return a CallbackData that can be used to cancel the callback.
    // A CallbackData is a struct that contains the callback and its ID (which you define in defs.h).
    // Most platforms choose to allocate a new CallbackData on the heap and free() is when the callback is either cancelled or
    // returns zero.
    // However, a non-rescheduling callback will never be cancelled.

    // So, in summary:
    // 1. Schedule a Callback `callback` to be called in `ms` milliseconds.
    // 2. Return a CallbackData that can be used to cancel the callback.
    // 3. Make sure to handle the Callback's return value correctly.
    // 4. Make sure to handle your memory correctly.

    // This function can return NULL if the callback could not be scheduled.
}

void cancel_callback(CallbackData *data) {
    // This function should cancel a callback with the given data.
    // Most platforms use the ID in the CallbackData to cancel the callback.
    // If your platform is using heap allocation for CallbackData, remember to free() it here.
}

void sleep_us_blocking(u64 us) {
    // This function should not return until `us` microseconds have passed (blocking).
    // This can be achieved through hardware sleep, busy-wait, or other means.
}
