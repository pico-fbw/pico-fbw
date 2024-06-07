/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdlib.h>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <sys/time.h>
    #include <unistd.h>
    #include "time_apple.h" // Compatibility layer for <time.h> on macOS
#elif defined(__linux__)
    #include <signal.h>
    #include <sys/time.h>
    #include <time.h>
    #include <unistd.h>
#endif

#include "sys_shared.h"

#include "platform/time.h"

#if defined(_WIN32)

VOID CALLBACK callback_to_WAITORTIMERCALLBACK(PVOID lp_param, BOOLEAN timer_or_wait_fired) {
    CallbackData *data = (CallbackData *)lp_param;
    if (!data)
        return;
    i32 reschedule = data->callback(data->data);
    if (reschedule <= 0) {
        DeleteTimerQueueTimer(NULL, data->id, NULL);
        free(data);
    }
    (void)timer_or_wait_fired;
}

#elif defined(__APPLE__) || defined(__linux__)

static void callback_to_sigevent(union sigval sv); // Forward declaration

/**
 * Creates a system timer from the given data and schedules it to fire after `ms` milliseconds.
 * @param data the CallbackData struct that the timer is associated with
 * @param ms the time after which the timer should fire (in ms)
 * @return true if the timer was successfully created and scheduled
 */
static bool create_timer(CallbackData *data, u32 ms) {
    struct sigevent sev = {
        .sigev_notify = SIGEV_THREAD,
        .sigev_notify_function = callback_to_sigevent,
        .sigev_value.sival_ptr = data,
    };
    if (timer_create(CLOCK_REALTIME, &sev, &data->id) != 0) {
        free(data);
        return false;
    }
    // clang-format off
    struct itimerspec its = {
        .it_interval = {0},
        .it_value = {ms / 1000, (ms % 1000) * 1000000}
    };
    // clang-format on
    if (timer_settime(data->id, 0, &its, NULL) != 0) {
        timer_delete(data->id);
        free(data);
        return false;
    }
    return true;
}

// Wrapper function to convert the callback signature from `i32 (*)()` to `void (*)(union sigval)`
static void callback_to_sigevent(union sigval sv) {
    CallbackData *data = sv.sival_ptr;
    if (!data)
        return;
    i32 reschedule = data->callback(data->data);
    if (reschedule > 0) {
        timer_delete(data->id);
        if (!create_timer(data, reschedule))
            free(data);
    } else {
        timer_delete(data->id);
        free(data);
    }
}

#endif // defined(__APPLE__) || defined(__linux__)

u64 time_us() {
#if defined(_WIN32)
    LARGE_INTEGER tNow;
    QueryPerformanceCounter(&tNow);
    return (tNow.QuadPart - tStart.QuadPart) * 1000000 / tFreq.QuadPart; // tStart and tFreq are set in sys.c
#elif defined(__APPLE__) || defined(__linux__)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000 + tv.tv_usec) - tStart; // tStart is set in sys.c
#endif
}

CallbackData *callback_in_ms(u32 ms, Callback callback, void *data) {
    CallbackData *cbData = malloc(sizeof(CallbackData));
    if (!cbData)
        return NULL;
    cbData->callback = callback;
    cbData->data = data;
#if defined(_WIN32)
    if (!CreateTimerQueueTimer(&cbData->id, NULL, callback_to_WAITORTIMERCALLBACK, cbData, ms, ms, 0)) {
        free(cbData);
        return NULL;
    }
#elif defined(__APPLE__) || defined(__linux__)
    if (!create_timer(cbData, ms)) {
        free(cbData);
        return NULL;
    }
#endif
    return cbData;
}

void cancel_callback(CallbackData *data) {
    if (!data)
        return;
#if defined(_WIN32)
    DeleteTimerQueueTimer(NULL, data->id, NULL);
#elif defined(__APPLE__) || defined(__linux__)
    timer_delete(data->id);
#endif
    free(data);
}

void sleep_us_blocking(u64 us) {
#if defined(_WIN32)
    Sleep(us / 1000);
#elif defined(__APPLE__) || defined(__linux__)
    usleep(us);
#endif
}
