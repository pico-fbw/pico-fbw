/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

// TODO: refactor, make less convoluted, easier to use
// rethink how sleeps/concurrency is handled (because that's why this file exists in the first place
// and it's not implemented particularly well at the moment)

#include "platform/defs.h"
#include "platform/gpio.h"
#include "platform/sys.h"
#include "platform/time.h"
#include "platform/wifi.h"

#include "ctrl/aircraft.h"
#include "ctrl/switch.h"
#include "io/gps.h"
#include "io/imu.h"
#include "sys/api/api.h"
#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"

#include "runtime.h"

#define MIN_LOOP_RATE 200 // Minimum safe loop update rate

static u32 loopCount = 0, loopRate = 0;
static bool loopRateWarned = false;

// Calculates the loop update rate and stores it in `loopRate`.
static i32 calc_update_rate(void *data) {
    // Since 1 second has passed since loopCount was reset, loopCount is effectively the number of loops per second
    loopRate = loopCount;
    loopCount = 0;
    if (loopRate < MIN_LOOP_RATE && !loopRateWarned) {
        log_message(TYPE_WARNING, "Loop update rate is low: %u Hz. Performance may be degraded!", 250, 0, true);
        loopRateWarned = true;
    }
    return 1000; // Run again in 1s
    (void)data;
}

void runtime_loop_begin() {
    callback_in_ms(1000, calc_update_rate, NULL);
}

void runtime_loop(bool update_aircraft) {
    // Update the mode switch's position
    switch_update();
    // Update sensors
    if (imu.ready) {
        imu.update();
    }
    if (gps.is_supported()) {
        gps.update();
    }
    // Run the current mode's code, if applicable
    if (update_aircraft) {
        aircraft_update();
    }
    // Respond to any new API calls
    if (config.general[GENERAL_API_ENABLED]) {
        api_poll();
    }
    // Platform-specific and system tasks
#if PLATFORM_SUPPORTS_WIFI
    if ((WifiEnabled)config.general[GENERAL_WIFI_ENABLED] != WIFI_DISABLED && !aircraft_wifi_deinitialized()) {
        wifi_periodic();
    }
#endif
    sys_periodic();
    loopCount++;
}

void runtime_loop_minimal() {
    // Update the minimal amount of systems required to keep the plane in the air
    aircraft_update();
    sys_periodic();
}

void runtime_sleep_ms(u32 ms, bool update_aircraft) {
    Timestamp wakeup_time = timestamp_in_ms(ms);
    while (!timestamp_reached(&wakeup_time)) {
        runtime_loop(update_aircraft);
    }
}
