/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "platform/defs.h"
#include "platform/time.h"
#include "platform/wifi.h"

#include "io/aahrs.h"
#include "io/gps.h"
#include "io/receiver.h"
#include "io/servo.h"
#include "modes/auto.h"
#include "modes/direct.h"
#include "modes/hold.h"
#include "modes/launch.h"
#include "modes/normal.h"
#include "modes/tune.h"
#include "sys/configuration.h"
#include "sys/flightplan.h"
#include "sys/log.h"
#include "sys/print.h"

#include "aircraft.h"

// Speed threshold to determine if the aircraft is flying (kts)
#define SPEED_FLYING_THRESHOLD 5
// The highest amount of time that the aircraft can still be considered flying after the last control input (s)
#define STILL_FLYING_TIMEOUT 15

static Timestamp lastNonzeroInput; // Last time a control input was detected

/**
 * Determines whether or not the aircraft is currently flying.
 * @return true if the aircraft is assumed to be flying, false if it is likely grounded
 * @note Must be called somewhat frequently to ensure accuracy.
 */
static bool is_flying() {
    if (GPS_OK()) {
        // We have GPS data, speed is a good infdication of whether or not we're flying
        return gps.speed >= SPEED_FLYING_THRESHOLD;
    } else {
        // Have there been any recent control inputs?
        if (USER_INPUTTING()) {
            lastNonzeroInput = timestamp_now();
        }
        return time_since_s(&lastNonzeroInput) < STILL_FLYING_TIMEOUT;
    }
}

/**
 * @param mode the (current) mode to deinitialize
 */
static void deinit_mode(Mode mode) {
    switch (mode) {
        case MODE_NORMAL:
            normal_deinit();
            break;
        case MODE_TUNE:
            tune_deinit();
            break;
        default:
            break;
    }
    aircraft.mode = MODE_INVALID;
}

/**
 * @param mode the mode to initialize
 */
static void init_mode(Mode mode) {
    switch (mode) {
        default:
        case MODE_DIRECT:
            aircraft.mode = MODE_DIRECT;
            break;
        LAUNCH:
        case MODE_LAUNCH:
            launch_init(mode);
            aircraft.mode = MODE_LAUNCH;
            break;
        NORMAL:
        case MODE_NORMAL:
            // Initiate an autolaunch if necessary
            if ((bool)config.general[GENERAL_LAUNCHASSIST_ENABLED]) {
                printsys(aircraft, "initiating launch assist");
                goto LAUNCH;
            }
            normal_init();
            aircraft.mode = MODE_NORMAL;
            break;
        case MODE_AUTO:
            if (!tune_is_tuned() && (bool)config.general[GENERAL_AUTOTUNE_ENABLED]) {
                printsys(aircraft, "initiating autotune");
                goto TUNE;
            }
            if (!GPS_OK()) {
                // GPS is required to be safe for auto and hold modes, fallback to normal mode
                printsys(aircraft, "WARNING: GPS is required for auto mode, falling back to normal mode");
                goto NORMAL;
            }
            if ((bool)config.general[GENERAL_LAUNCHASSIST_ENABLED]) {
                printsys(aircraft, "initiating launch assist");
                goto LAUNCH;
            }
            if (!auto_init()) {
                printsys(aircraft, "WARNING: failed to initialize auto mode, falling back to normal mode");
                goto NORMAL;
            }
            aircraft.mode = MODE_AUTO;
            break;
        TUNE:
        case MODE_TUNE:
            if (tune_is_tuned()) {
                printsys(aircraft, "already tuned, falling back to normal mode");
                goto NORMAL;
            }
            tune_init();
            aircraft.mode = MODE_TUNE;
            break;
        case MODE_HOLD:
            if (!GPS_OK()) {
                printsys(aircraft, "WARNING: GPS is required for hold mode, falling back to normal mode");
                goto NORMAL;
            }
            if (!hold_init()) {
                printsys(aircraft, "WARNING: failed to initialize hold mode, falling back to normal mode");
                goto NORMAL;
            }
            aircraft.mode = MODE_HOLD;
            break;
    }
}

const char *mode_to_string(Mode mode) {
    switch (mode) {
        case MODE_LAUNCH:
            return "launch";
        case MODE_DIRECT:
            return "direct";
        case MODE_NORMAL:
            return "normal";
        case MODE_AUTO:
            return "auto";
        case MODE_TUNE:
            return "tune";
        case MODE_HOLD:
            return "hold";
        default:
            return "invalid";
    }
}

void update() {
    switch (aircraft.mode) {
        default:
        case MODE_DIRECT:
            direct_update();
            break;
        case MODE_LAUNCH:
            launch_update();
            break;
        case MODE_NORMAL:
            normal_update();
            break;
        case MODE_AUTO:
            auto_update();
            break;
        case MODE_TUNE:
            tune_update();
            break;
        case MODE_HOLD:
            hold_update();
            break;
    }
    aircraft.isFlying = is_flying();
}

void change_to(Mode new_mode) {
    // Deinit the current mode
    printsys(aircraft, "exiting %s mode", mode_to_string(aircraft.mode));
    deinit_mode(aircraft.mode);
    // All modes (except for direct) require AAHRS so make sure that's all good
    if (!aircraft.aahrsSafe && new_mode != MODE_DIRECT) {
        printsys(aircraft, "AAHRS has failed, entering direct mode!");
        log_message(TYPE_ERROR, "AAHRS has failed!", 250, 0, true);
        aircraft.mode = MODE_DIRECT;
        return;
    }
    // Init the new mode
    printsys(aircraft, "trying to enter %s mode", mode_to_string(new_mode));
    init_mode(new_mode);
    printsys(aircraft, "entered %s mode", mode_to_string(aircraft.mode));
#if PLATFORM_SUPPORTS_WIFI
    if (!aircraft.wifiDeinitialized && aircraft.isFlying) {
        // We're now airborne, so wifi is no longer needed
        if (!wifi_disable()) {
            printsys(network, "WARNING: failed to disable wifi!");
        }
        printsys(network, "wifi disabled");
        aircraft.wifiDeinitialized = true;
    }
#endif
}

void set_aahrs_safe(bool state) {
    if (state == aircraft.aahrsSafe) {
        return; // Nothing to do
    }
    aircraft.aahrsSafe = state;
    if (!aircraft.aahrsSafe) {
        // Change to direct mode as it doesn't require AAHRS, and deinit
        change_to(MODE_DIRECT);
        aahrs.deinit();
        printsys(aircraft, "AAHRS set as unsafe");
        return;
    }
    // Last-ditch attempt to re-init AAHRS if it's not already
    if (!aahrs.isInitialized && !aahrs.init()) {
        log_message(TYPE_ERROR, "AAHRS initialization failed!", 1000, 0, false);
        change_to(MODE_DIRECT);
        return;
    }
    printsys(aircraft, "AAHRS set as safe");
}

void set_gps_safe(bool state) {
    if (state == aircraft.gpsSafe) {
        return;
    }
    aircraft.gpsSafe = state;
    if (aircraft.gpsSafe) {
        printsys(aircraft, "GPS set as safe");
        log_clear(TYPE_INFO);
        return;
    }
    printsys(aircraft, "GPS set as unsafe");
    if (aircraft.mode == MODE_AUTO || aircraft.mode == MODE_HOLD) {
        // Return to normal mode if GPS is deemed unsafe in Auto or Hold modes (require GPS)
        change_to(MODE_NORMAL);
    }
}

// clang-format off
Aircraft aircraft = {
    .mode = MODE_DIRECT,
    .isFlying = false,
#if PLATFORM_SUPPORTS_WIFI
    .wifiDeinitialized = false,
#endif
    .aahrsSafe = false,
    .gpsSafe = false,
    .update = update,
    .change_to = change_to,
    .set_aahrs_safe = set_aahrs_safe,
    .set_gps_safe = set_gps_safe,
};
// clang-format on
