/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
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
        if (USER_INPUTTING())
            lastNonzeroInput = timestamp_now();
        return time_since_s(&lastNonzeroInput) < STILL_FLYING_TIMEOUT;
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
    switch (aircraft.mode) {
        default:
        case MODE_DIRECT:
            printfbw(aircraft, "exiting direct mode");
            break;
        case MODE_LAUNCH:
            printfbw(aircraft, "exiting launch mode");
            break;
        case MODE_NORMAL:
            printfbw(aircraft, "exiting normal mode");
            normal_deinit();
            break;
        case MODE_AUTO:
            printfbw(aircraft, "exiting auto mode");
            break;
        case MODE_TUNE:
            tune_deinit();
            printfbw(aircraft, "exiting tune mode");
            break;
        case MODE_HOLD:
            printfbw(aircraft, "exiting hold mode");
            break;
    }
    // All modes (except for direct) require AAHRS so make sure that's all good
    if (!aircraft.aahrsSafe) {
        printfbw(aircraft, "AAHRS has failed, entering direct mode!");
        log_message(TYPE_ERROR, "AAHRS has failed!", 250, 0, true);
        aircraft.mode = MODE_DIRECT;
        return;
    }
    // Init the new mode
    switch (new_mode) {
        default:
        case MODE_DIRECT:
            printfbw(aircraft, "entering direct mode");
            aircraft.mode = MODE_DIRECT;
            break;
        LAUNCH:
        case MODE_LAUNCH:
            printfbw(aircraft, "entering launch mode");
            launch_init(new_mode);
            aircraft.mode = MODE_LAUNCH;
            break;
        NORMAL:
        case MODE_NORMAL:
            if ((bool)config.general[GENERAL_LAUNCHASSIST_ENABLED])
                goto LAUNCH; // Initiate an autolaunch if necessary
            printfbw(aircraft, "entering normal mode");
            normal_init();
            aircraft.mode = MODE_NORMAL;
            break;
        case MODE_AUTO:
            if (!tune_is_tuned())
                goto TUNE; // Automatically enter tune mode if necessary
            if (!GPS_OK())
                goto NORMAL; // GPS is required to be safe for auto and hold modes, fallback to normal mode
            if ((bool)config.general[GENERAL_LAUNCHASSIST_ENABLED])
                goto LAUNCH;
            // Check to see if we should calibrate the altitude offset
            if (flightplan_was_parsed()) {
                if (flightplan_get()->alt_samples > 0)
                    gps.calibrate_alt_offset(flightplan_get()->alt_samples);
            }
            printfbw(aircraft, "entering auto mode");
            if (auto_init()) {
                aircraft.mode = MODE_AUTO;
            } else
                goto NORMAL;
            break;
        TUNE:
        case MODE_TUNE:
            if (tune_is_tuned())
                goto NORMAL;
            printfbw(aircraft, "entering tune mode");
            tune_init();
            aircraft.mode = MODE_TUNE;
            break;
        case MODE_HOLD:
            if (!GPS_OK())
                goto NORMAL;
            printfbw(aircraft, "entering hold mode");
            if (hold_init())
                aircraft.mode = MODE_HOLD;
            else
                goto NORMAL;
            break;
    }
#if PLATFORM_SUPPORTS_WIFI
    static bool deinitializedWifi = false;
    if (aircraft.isFlying && !deinitializedWifi) {
        // We're now airborne, so wifi is no longer needed
        if (!wifi_disable())
            printfbw(network, "WARNING: failed to disable wifi!");
        deinitializedWifi = true;
    }
#endif
}

void set_aahrs_safe(bool state) {
    if (state == aircraft.aahrsSafe)
        return;
    aircraft.aahrsSafe = state;
    if (state) {
        if (!aahrs.isInitialized) {
            if (!aahrs.init()) {
                log_message(TYPE_ERROR, "AAHRS initialization failed!", 1000, 0, false);
                change_to(MODE_DIRECT);
                return;
            }
        }
        printfbw(aircraft, "AAHRS set as safe");
    } else {
        // Change to direct mode as it doesn't require AAHRS, and deinit
        change_to(MODE_DIRECT);
        aahrs.deinit();
        printfbw(aircraft, "AAHRS set as unsafe");
    }
}

void set_gps_safe(bool state) {
    if (state == aircraft.gpsSafe)
        return;
    aircraft.gpsSafe = state;
    if (state) {
        printfbw(aircraft, "GPS set as safe");
        log_clear(TYPE_INFO);
    } else {
        printfbw(aircraft, "GPS set as unsafe");
        if (aircraft.mode == MODE_AUTO || aircraft.mode == MODE_HOLD)
            change_to(MODE_NORMAL); // Return to normal mode if GPS is deemed unsafe in Auto or Hold modes (require GPS)
    }
}

// clang-format off
Aircraft aircraft = {
    .mode = MODE_DIRECT,
    .isFlying = false,
    .aahrsSafe = false,
    .gpsSafe = false,
    .update = update,
    .change_to = change_to,
    .set_aahrs_safe = set_aahrs_safe,
    .set_gps_safe = set_gps_safe
};
// clang-format on
