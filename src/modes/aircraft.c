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

#include "sys/log.h"
#include "sys/print.h"

#include "wifly/wifly.h"

#include "aircraft.h"

void update() {
    switch (aircraft.mode) {
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

void changeTo(Mode newMode) {
    // Run deinit code for aircraft.mode and then run init code for newMode
    switch (aircraft.mode) {
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
            printfbw(modes, "exiting tune mode");
            break;
        case MODE_HOLD:
            printfbw(modes, "exiting hold mode");
            break;
    }
    if (aircraft.AAHRSSafe) {
        switch (newMode) {
            DIRECT:
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
            AUTO:
            case MODE_AUTO:
                // Automatically enter tune mode if necessary
                if (!tune_is_tuned())
                    goto TUNE;
                // TODO: have a way for auto mode to re-engage if the gps becomes safe again; this is usually due to bad DOP which fixes itself over time
                if (gps.is_supported() && aircraft.GPSSafe) {
                    // Check to see if we have to calibrate the altitude offset
                    if (wifly_getNumAltSamples() > 0) {
                        gps.calibrate_alt_offset(wifly_getNumAltSamples());
                    }
                    printfbw(modes, "entering auto mode");
                    if (auto_init()) {
                        aircraft.mode = MODE_AUTO;
                    } else goto NORMAL;
                } else goto NORMAL; // GPS is required to be safe for auto and hold modes, fallback to normal mode
                break;
            TUNE:
            case MODE_TUNE:
                if (!tune_is_tuned()) {
                    printfbw(modes, "entering tune mode");
                    aircraft.mode = MODE_TUNE;
                } else goto NORMAL;
                break;
            HOLD:
            case MODE_HOLD:
                if (gps.is_supported() && aircraft.GPSSafe) {
                    printfbw(modes, "entering hold mode");
                    if (hold_init()) {
                        aircraft.mode = MODE_HOLD;
                    } else goto NORMAL;
                } else goto NORMAL;
                break;
        }
    } else {
        printfbw(modes, "AAHRS has failed, entering direct mode!");
        log_message(ERROR, "AAHRS has failed!", 250, 0, true);
        aircraft.mode = MODE_DIRECT;
    }
}

void setAAHRSSafe(bool state) {
    if (state != aircraft.AAHRSSafe) {
        aircraft.AAHRSSafe = state;
        if (state) {
            printfbw(modes, "AAHRS set as safe");
        } else {
            // Change to direct mode as it doesn't require AAHRS, and deinit
            changeTo(MODE_DIRECT);
            aahrs.deinit();
            printfbw(modes, "AAHRS set as unsafe");
        }
    }
}

void setGPSSafe(bool state) {
    if (state != aircraft.GPSSafe) {
        aircraft.GPSSafe = state;
        if (state) {
            printfbw(modes, "GPS set as safe");
            log_clear(INFO);
        } else {
            printfbw(modes, "GPS set as unsafe");
            if (aircraft.mode == MODE_AUTO || aircraft.mode == MODE_HOLD) {
                changeTo(MODE_NORMAL); // Return to normal mode if GPS is deemed unsafe in Auto or Hold modes (require GPS)
            }
        }
    }
}

Aircraft aircraft = {
    .mode = MODE_DIRECT,
    .AAHRSSafe = false,
    .GPSSafe = false,
    .update = update,
    .changeTo = changeTo,
    .setAAHRSSafe = setAAHRSSafe,
    .setGPSSafe = setGPSSafe
};
