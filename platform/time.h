#pragma once

#include <stdbool.h>
#include "platform/defs.h"
#include "platform/types.h"

typedef i32 (*Callback)();

typedef struct CallbackData {
    Callback callback;
    __callback_id_t id;
} CallbackData;

typedef struct Timestamp {
    u64 us;
} Timestamp;

/**
 * @return the curren time since the system powered on, in microseconds
 */
u64 time_us();

/**
 * Schedules a callback to be called in `ms` milliseconds.
 * @param ms the number of milliseconds to wait before calling the callback
 * @param callback the callback to call
 * @return some `CallbackData` that can be used to cancel the callback, or NULL if the callback could not be scheduled
 * @note The Callback function must have the signature `i32 (*)()`, aka it must return an `i32` and be a function.
 * Within the function, returning a positive value will reschedule the callback in that many milliseconds, and returning 0 will
 * not reschedule the callback.
 */
CallbackData *callback_in_ms(u32 ms, Callback callback);

/**
 * Cancels a callback with the given ID.
 * @param id the ID of the callback to cancel
 * @note This function only works with repeating timers (aka timers that return a positive value in the callback).
 * Attempting to call this function on a non-repeating timer will result in undefined behavior.
 */
void cancel_callback(CallbackData *data);

/**
 * Sleeps for `us` microseconds, blocking the current core.
 * @param us the number of microseconds to sleep
 */
void sleep_us_blocking(u64 us);

/**
 * @return the current time since the system powered on, in milliseconds
 */
static inline u32 time_ms() {
    return time_us() / 1E3;
}

/**
 * @return the current time since the system powered on, in seconds
 */
static inline f32 time_s() {
    return time_us() / 1E6;
}

/**
 * Sleeps for `ms` milliseconds, blocking the current thread.
 * @param ms the number of milliseconds to sleep
 */
static inline void sleep_ms_blocking(u32 ms) {
    sleep_us_blocking(ms * 1000);
}

/**
 * Creates a Timestamp representing the current time.
 * @return the Timestamp
 */
static inline Timestamp timestamp_now() {
    return (Timestamp){.us = time_us()};
}

/**
 * Creates a Timestamp `us` microseconds from now.
 * @param us the number of microseconds from the current time to create the Timestamp
 * @return the Timestamp
 */
static inline Timestamp timestamp_in_us(u64 us) {
    return (Timestamp){.us = time_us() + us};
}

/**
 * Creates a Timestamp `ms` milliseconds from now.
 * @param ms the number of milliseconds from the current time to create the Timestamp
 * @return the Timestamp
 */
static inline Timestamp timestamp_in_ms(u32 ms) {
    return timestamp_in_us(ms * 1000);
}

/**
 * @param timestamp the Timestamp to check
 * @return the time since the Timestamp was created, in microseconds
 */
static inline u64 time_since_us(const Timestamp *timestamp) {
    return time_us() - timestamp->us;
}
/**
 * @param timestamp the Timestamp to check
 * @return the time since the Timestamp was created, in milliseconds
 */
static inline u32 time_since_ms(const Timestamp *timestamp) {
    return time_since_us(timestamp) / 1000;
}

/**
 * @param timestamp the Timestamp to check
 * @return the time since the Timestamp was created, in seconds
 */
static inline f32 time_since_s(const Timestamp *timestamp) {
    return time_since_us(timestamp) / 1E6;
}

/**
 * @param timestamp the Timestamp to check
 * @return true if the current time is greater than or equal to the Timestamp
 */
static inline bool timestamp_reached(const Timestamp *timestamp) {
    return time_us() >= timestamp->us;
}
