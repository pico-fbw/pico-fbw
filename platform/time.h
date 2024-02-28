#pragma once

#include <stdbool.h>
#include "platform/int.h"

typedef struct Timestamp {
    u64 us;
} Timestamp;

typedef i32 (*Callback)(u32 id);

/**
 * @return the current 64-bit time since the system powered on, in microseconds
 */
u64 time_us();

/**
 * Schedules a callback to be called in `ms` milliseconds.
 * @param ms the number of milliseconds to wait before calling the callback
 * @param callback the callback to call
 * @return the ID of the callback, which can be used to cancel the callback (-1 if the callback could not be scheduled)
 * @note The Callback function must have the signature `i32 (*)(u32)`, aka it must return an `i32` and have a `u32` as a
 * parameter. Returning 0 will not reschedule the callback, returning a positive value will reschedule the callback in that many
 * milliseconds, and returning a negative value will reschedule the callback that many milleseconds from when the callback was
 * originally scheduled.
 */
i32 callback_in_ms(u32 ms, Callback callback);

/**
 * Cancels a callback with the given ID.
 * @param id the ID of the callback to cancel
 * @return true if the callback was successfully cancelled
 */
bool cancel_callback(u32 id);

/**
 * Sleeps for `us` microseconds, blocking the current thread.
 * @param us the number of microseconds to sleep
 */
void sleep_us_blocking(u64 us);

/**
 * Sleeps for `ms` milliseconds, blocking the current thread.
 * @param ms the number of milliseconds to sleep
 */
inline void sleep_ms_blocking(u32 ms) { sleep_us_blocking(ms * 1000); }

/**
 * Creates a Timestamp representing the current time.
 * @return the Timestamp
 */
inline Timestamp timestamp_now() { return (Timestamp){.us = time_us()}; }

/**
 * Creates a Timestamp `us` microseconds from now.
 * @param us the number of microseconds from the current time to create the Timestamp
 * @return the Timestamp
 */
inline Timestamp timestamp_in_us(u64 us) { return (Timestamp){.us = time_us() + us}; }

/**
 * Creates a Timestamp `ms` milliseconds from now.
 * @param ms the number of milliseconds from the current time to create the Timestamp
 * @return the Timestamp
 */
inline Timestamp timestamp_in_ms(u32 ms) { return timestamp_in_us(ms * 1000); }

/**
 * @param timestamp the Timestamp to check
 * @return the time since the Timestamp was created, in microseconds
 */
inline u64 time_since_us(const Timestamp *timestamp) { return time_us() - timestamp->us; }
/**
 * @param timestamp the Timestamp to check
 * @return the time since the Timestamp was created, in milliseconds
 */
inline u32 time_since_ms(const Timestamp *timestamp) { return time_since_us(timestamp) / 1000; }

/**
 * @param timestamp the Timestamp to check
 * @return true if the current time is greater than or equal to the Timestamp
 */
inline bool timestamp_reached(const Timestamp *timestamp) { return time_us() >= timestamp->us; }
