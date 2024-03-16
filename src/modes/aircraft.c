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
            printfbw(modes, "exiting direct mode");
            break;
        case MODE_NORMAL:
            printfbw(modes, "exiting normal mode");
            normal_deinit();
            break;
        case MODE_AUTO:
            printfbw(modes, "exiting auto mode");
            break;
        case MODE_TUNE:
            tune_deinit();
            printfbw(modes, "exiting tune mode");
            break;
        case MODE_HOLD:
            printfbw(modes, "exiting hold mode");
            break;
    }
    if (aircraft.aahrsSafe) {
        switch (newMode) {
            default:
            case MODE_DIRECT:
                printfbw(modes, "entering direct mode");
                aircraft.mode = MODE_DIRECT;
                break;
            NORMAL:
            case MODE_NORMAL:
                printfbw(modes, "entering normal mode");
                normal_init();
                aircraft.mode = MODE_NORMAL;
                break;
            case MODE_AUTO:
                // Automatically enter tune mode if necessary
                if (!tune_is_tuned())
                    goto TUNE;
                // TODO: have a way for auto mode to re-engage if the gps becomes safe again; this is usually due to bad DOP
                // which fixes itself over time
                if (gps.is_supported() && aircraft.gpsSafe) {
                    // Check to see if we should calibrate the altitude offset
                    if (flightplan_was_parsed()) {
                        if (flightplan_get()->alt_samples > 0)
                            gps.calibrate_alt_offset(flightplan_get()->alt_samples);
                    }
                    printfbw(modes, "entering auto mode");
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
                    printfbw(modes, "entering tune mode");
                    tune_init();
                    aircraft.mode = MODE_TUNE;
                } else
                    goto NORMAL;
                break;
            case MODE_HOLD:
                if (gps.is_supported() && aircraft.gpsSafe) {
                    printfbw(modes, "entering hold mode");
                    if (hold_init()) {
                        aircraft.mode = MODE_HOLD;
                    } else
                        goto NORMAL;
                } else
                    goto NORMAL;
                break;
        }
    } else {
        printfbw(modes, "AAHRS has failed, entering direct mode!");
        log_message(ERROR, "AAHRS has failed!", 250, 0, true);
        aircraft.mode = MODE_DIRECT;
    }
}

void set_aahrs_safe(bool state) {
    if (state != aircraft.aahrsSafe) {
        aircraft.aahrsSafe = state;
        if (state) {
            printfbw(modes, "AAHRS set as safe");
        } else {
            // Change to direct mode as it doesn't require AAHRS, and deinit
            change_to(MODE_DIRECT);
            aahrs.deinit();
            printfbw(modes, "AAHRS set as unsafe");
        }
    }
}

void set_gps_safe(bool state) {
    if (state != aircraft.gpsSafe) {
        aircraft.gpsSafe = state;
        if (state) {
            printfbw(modes, "GPS set as safe");
            log_clear(INFO);
        } else {
            printfbw(modes, "GPS set as unsafe");
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
