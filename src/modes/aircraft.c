/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "io/aahrs.h"
#include "io/gps.h"
#include "io/servo.h"

#include "modes/auto.h"
#include "modes/direct.h"
#include "modes/hold.h"
#include "modes/normal.h"
#include "modes/tune.h"

#include "sys/flightplan.h"
#include "sys/log.h"
#include "sys/print.h"

#include "aircraft.h"

void update() {
    switch (aircraft.mode) {
        default:
        case MODE_DIRECT:
            direct_update();
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
}

// TODO: autolaunch(detect accel and fly away)->normal/auto mode?

void change_to(Mode newMode) {
    // Run deinit code for aircraft.mode and then run init code for newMode
    switch (aircraft.mode) {
        default:
        case MODE_DIRECT:
            printfbw(aircraft, "exiting direct mode");
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
    if (aircraft.aahrsSafe) {
        switch (newMode) {
            default:
            case MODE_DIRECT:
                printfbw(aircraft, "entering direct mode");
                aircraft.mode = MODE_DIRECT;
                break;
            NORMAL:
            case MODE_NORMAL:
                printfbw(aircraft, "entering normal mode");
                normal_init();
                aircraft.mode = MODE_NORMAL;
                break;
            case MODE_AUTO:
                // Automatically enter tune mode if necessary
                if (!tune_is_tuned())
                    goto TUNE;
                if (gps.is_supported() && aircraft.gpsSafe) {
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
                } else
                    goto NORMAL; // GPS is required to be safe for auto and hold modes, fallback to normal mode
                break;
            TUNE:
            case MODE_TUNE:
                if (!tune_is_tuned()) {
                    printfbw(aircraft, "entering tune mode");
                    tune_init();
                    aircraft.mode = MODE_TUNE;
                } else
                    goto NORMAL;
                break;
            case MODE_HOLD:
                if (gps.is_supported() && aircraft.gpsSafe) {
                    printfbw(aircraft, "entering hold mode");
                    if (hold_init()) {
                        aircraft.mode = MODE_HOLD;
                    } else
                        goto NORMAL;
                } else
                    goto NORMAL;
                break;
        }
    } else {
        printfbw(aircraft, "AAHRS has failed, entering direct mode!");
        log_message(ERROR, "AAHRS has failed!", 250, 0, true);
        aircraft.mode = MODE_DIRECT;
    }
}

void set_aahrs_safe(bool state) {
    if (state != aircraft.aahrsSafe) {
        aircraft.aahrsSafe = state;
        if (state) {
            if (!aahrs.isInitialized) {
                if (!aahrs.init()) {
                    log_message(ERROR, "AAHRS initialization failed!", 1000, 0, false);
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
}

void set_gps_safe(bool state) {
    if (state != aircraft.gpsSafe) {
        aircraft.gpsSafe = state;
        if (state) {
            printfbw(aircraft, "GPS set as safe");
            log_clear(INFO);
        } else {
            printfbw(aircraft, "GPS set as unsafe");
            if (aircraft.mode == MODE_AUTO || aircraft.mode == MODE_HOLD) {
                change_to(MODE_NORMAL); // Return to normal mode if GPS is deemed unsafe in Auto or Hold modes (require GPS)
            }
        }
    }
}

// clang-format off
Aircraft aircraft = {
    .mode = MODE_DIRECT,
    .aahrsSafe = false,
    .gpsSafe = false,
    .update = update,
    .change_to = change_to,
    .set_aahrs_safe = set_aahrs_safe,
    .set_gps_safe = set_gps_safe
};
// clang-format on
