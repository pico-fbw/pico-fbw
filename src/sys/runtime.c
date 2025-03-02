/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/defs.h"
#include "platform/gpio.h"
#include "platform/sys.h"
#include "platform/time.h"
#include "platform/wifi.h"

#include "ctrl/aircraft.h"
#include "ctrl/switch.h"
#include "io/aahrs.h"
#include "io/gps.h"
#include "sys/api/api.h"
#include "sys/configuration.h"

#include "runtime.h"

void runtime_loop(bool update_aircraft) {
    // Update the mode switch's position
    switch_update();
    // Update sensors
    if (aahrs.isInitialized) {
        aahrs.update();
    }
    if (gps.is_supported()) {
        gps.update();
    }
    // Run the current mode's code, if applicable
    if (update_aircraft) {
        aircraft.update();
    }
    // Respond to any new API calls
    if (config.general[GENERAL_API_ENABLED]) {
        api_poll();
    }
    // Platform-specific and system tasks
#if PLATFORM_SUPPORTS_WIFI
    if ((WifiEnabled)config.general[GENERAL_WIFI_ENABLED] != WIFI_DISABLED && !aircraft.wifiDeinitialized) {
        wifi_periodic();
    }
#endif
    sys_periodic();
}

void runtime_loop_minimal() {
    // Update the minimal amount of systems required to keep the plane in the air
    aircraft.update();
    sys_periodic();
}

void runtime_sleep_ms(u32 ms, bool update_aircraft) {
    Timestamp wakeup_time = timestamp_in_ms(ms);
    while (!timestamp_reached(&wakeup_time)) {
        runtime_loop(update_aircraft);
    }
}
