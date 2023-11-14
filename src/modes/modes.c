/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/time.h"

#include "../io/aahrs.h"
#include "../io/flash.h"
#include "../io/gps.h"
#include "../io/servo.h"

#include "../wifly/wifly.h"

#include "auto.h"
#include "direct.h"
#include "hold.h"
#include "normal.h"
#include "tune.h"

#include "../sys/log.h"

#include "modes.h"

static Mode currentMode = MODE_DIRECT;
static bool aahrsSafe = false;
static bool gpsSafe = false;

void update() {
    switch(currentMode) {
        case MODE_DIRECT:
            mode_direct();
            break;   
        case MODE_NORMAL:
            mode_normal();
            break; 
        case MODE_AUTO:
            mode_auto();
            break;
        case MODE_TUNE:
            mode_tune();
            break;
        case MODE_HOLD:
            mode_hold();
            break;
    }
}

void changeTo(Mode newMode) {
    // Run deinit code for currentMode and then run init code for newMode
    switch (currentMode) {
        case MODE_DIRECT:
            if (print.fbw) printf("[modes] exiting direct mode\n");
            break;
        case MODE_NORMAL:
            if (print.fbw) printf("[modes] exiting normal mode\n");
            mode_normalDeinit();
            break;
        case MODE_AUTO:
            if (print.fbw) printf("[modes] exiting auto mode\n");
            break;
        case MODE_TUNE:
            if (print.fbw) printf("[modes] exiting tune mode\n");
            break;
        case MODE_HOLD:
            if (print.fbw) printf("[modes] exiting hold mode\n");
            break;
    }
    if (aahrsSafe) {
        switch (newMode) {
            case MODE_DIRECT:
                if (print.fbw) printf("[modes] entering direct mode\n");
                currentMode = MODE_DIRECT;
                break;
            case MODE_NORMAL:
                // Automatically enter tune mode if necessary
                if (!mode_tuneisCalibrated()) {
                    changeTo(MODE_TUNE);
                    return;
                }
                if (print.fbw) printf("[modes] entering normal mode\n");
                mode_normalInit();
                currentMode = MODE_NORMAL;
                break;
            case MODE_AUTO:
                if (!mode_tuneisCalibrated()) {
                    changeTo(MODE_TUNE);
                    return;
                }
                if ((GPSCommandType)flash.sensors[SENSORS_GPS_COMMAND_TYPE] != GPS_COMMAND_TYPE_NONE) {
                    // TODO: have a way for auto mode to re-engage if the gps becomes safe again; this is usually due to bad DOP which fixes itself over time
                    if (gpsSafe) {
                        // Check to see if we have to calibrate the GPS alt offset
                        if (wifly_getNumAltSamples() > 0) {
                            gps_calibrateAltOffset(wifly_getNumAltSamples());
                        }
                        if (print.fbw) printf("[modes] entering auto mode\n");
                        if (mode_autoInit()) {
                            currentMode = MODE_AUTO;
                        } else {
                            changeTo(MODE_NORMAL);
                            return;
                        }
                    } else {
                        // GPS is required to be safe for auto and hold modes, fallback to normal mode
                        changeTo(MODE_NORMAL);
                        return;
                    }
                } else {
                    changeTo(MODE_NORMAL);
                    return;
                }
                break;
            case MODE_TUNE:
                if (!mode_tuneisCalibrated()) {
                    if (print.fbw) printf("[modes] entering tune mode\n");
                    currentMode = MODE_TUNE;
                } else {
                    changeTo(MODE_NORMAL);
                    return;
                }
                break;
            case MODE_HOLD:
                if (gpsSafe) {
                    if (print.fbw) printf("[modes] entering hold mode\n");
                    currentMode = MODE_HOLD;
                } else {
                    changeTo(MODE_NORMAL);
                    return;
                }
                break;
        }
    } else {
        if (print.fbw) printf("[modes] entering direct mode\n");
        log_message(ERROR, "AAHRS has failed, entering direct mode!", 250, 0, true);
        currentMode = MODE_DIRECT;
    }
}

Mode mode() { return currentMode; }

void setAAHRSSafe(bool state) {
    if (state != aahrsSafe) {
        aahrsSafe = state;
        if (state) {
            if (print.fbw) printf("[modes] AAHRS set as safe\n");
        } else {
            // Change to direct mode as it doesn't require AAHRS, and deinit
            changeTo(MODE_DIRECT);
            aahrs_deinit();
            if (print.fbw) printf("[modes] AAHRS set as unsafe\n");
        }
    }
}

bool AAHRSSafe() { return aahrsSafe; }

void setGPSSafe(bool state) {
    if (state != gpsSafe) {
        gpsSafe = state;
        if (state) {
            if (print.fbw) printf("[modes] GPS set as safe\n");
            log_clear(INFO);
        } else {
            if (print.fbw) printf("[modes] GPS set as unsafe\n");
            if (currentMode == MODE_AUTO || currentMode == MODE_HOLD) {
                changeTo(MODE_NORMAL); // Return to normal mode if GPS is deemed unsafe in Auto or Hold modes (require GPS)
            }
        }
    }
}

bool GPSSafe() { return gpsSafe; }

Aircraft aircraft = {
    .update = update,
    .changeTo = changeTo,
    .mode = mode,
    .setAAHRSSafe = setAAHRSSafe,
    .AAHRSSafe = AAHRSSafe,
    .setGPSSafe = setGPSSafe,
    .GPSSafe = GPSSafe
};
