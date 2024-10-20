/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/defs.h"
#include "platform/gpio.h"
#include "platform/sys.h"
#include "platform/time.h"
#include "platform/wifi.h"

#include "io/aahrs.h"
#include "io/gps.h"
#include "io/receiver.h"

#include "modes/aircraft.h"

#include "sys/api/api.h"
#include "sys/configuration.h"
#include "sys/flightplan.h"

#include "runtime.h"

// clang-format off
typedef enum SwitchPosition {
    SWITCH_POSITION_LOW,
    SWITCH_POSITION_MID,
    SWITCH_POSITION_HIGH,
} SwitchPosition;
// clang-format on

static SwitchPosition lastPos;

static SwitchPosition deg_to_pos(f32 deg) {
    switch ((SwitchType)config.general[GENERAL_SWITCH_TYPE]) {
        case SWITCH_TYPE_2_POS:
            if (deg < 90)
                return SWITCH_POSITION_LOW;
            else
                return SWITCH_POSITION_HIGH;
        default:
        case SWITCH_TYPE_3_POS:
            if (deg < 45)
                return SWITCH_POSITION_LOW;
            else if (deg > 135)
                return SWITCH_POSITION_HIGH;
            else
                return SWITCH_POSITION_MID;
    }
}

static void switch_update() {
    SwitchPosition pos = deg_to_pos(receiver_get((u32)config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEGREE));
    // The mode will only be changed when the user moves the switch; the system's mode changes can persist and won't instantly
    // be overrided by the switch
    if (lastPos != pos) {
        switch (pos) {
            case SWITCH_POSITION_LOW:
                aircraft.change_to(MODE_DIRECT);
                break;
            case SWITCH_POSITION_MID:
                aircraft.change_to(MODE_NORMAL);
                break;
            case SWITCH_POSITION_HIGH:
                switch ((SwitchType)config.general[GENERAL_SWITCH_TYPE]) {
                    case SWITCH_TYPE_2_POS:
                        // For 2-position switches, auto-select auto or normal mode based on if a flight plan is present or not
                        if (flightplan_was_parsed())
                            aircraft.change_to(MODE_AUTO);
                        else
                            aircraft.change_to(MODE_NORMAL);
                        break;
                    case SWITCH_TYPE_3_POS:
                        aircraft.change_to(MODE_AUTO);
                        break;
                }
                break;
        }
        lastPos = pos;
    }
}

void runtime_loop(bool update_aircraft) {
    // Update the mode switch's position, update sensors, run the current mode's code, respond to any new API calls, and run
    // platform-specific system tasks
    switch_update();
    if (aahrs.isInitialized)
        aahrs.update();
    if (gps.is_supported())
        gps.update();
    if (update_aircraft)
        aircraft.update();
    if ((bool)config.general[GENERAL_API_ENABLED])
        api_poll();
#if PLATFORM_SUPPORTS_WIFI
    if ((WifiEnabled)config.general[GENERAL_WIFI_ENABLED] != WIFI_DISABLED)
        wifi_periodic();
#endif
    sys_periodic();
}

void runtime_loop_minimal() {
    // Update the minimal amount of systems required to keep the plane in the air, this is usually called after a watchdog event
    aircraft.update();
    sys_periodic();
}

void runtime_sleep_ms(u32 ms, bool update_aircraft) {
    Timestamp wakeup_time = timestamp_in_ms(ms);
    while (!timestamp_reached(&wakeup_time))
        runtime_loop(update_aircraft);
}
