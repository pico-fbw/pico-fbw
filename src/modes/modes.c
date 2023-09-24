/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/time.h"

#include "../io/error.h"
#include "../io/gps.h"
#include "../io/imu.h"
#include "../io/servo.h"

#include "../wifly/wifly.h"

#include "auto.h"
#include "direct.h"
#include "hold.h"
#include "normal.h"
#include "tune.h"

#include "../sys/config.h"

#include "modes.h"

static Mode currentMode = MODE_DIRECT;
static bool imuDataSafe = false;
static bool gpsDataSafe = false;

void toMode(Mode newMode) {
    // Run deinit code for currentMode and then run init code for newMode
    switch (currentMode) {
        case MODE_DIRECT:
            if (config.debug.debug_fbw) printf("[modes] exiting direct mode\n");
            break;
        case MODE_NORMAL:
            if (config.debug.debug_fbw) printf("[modes] exiting normal mode\n");
            mode_normalDeinit();
            break;
        case MODE_AUTO:
            if (config.debug.debug_fbw) printf("[modes] exiting auto mode\n");
            break;
        case MODE_TUNE:
            if (config.debug.debug_fbw) printf("[modes] exiting tune mode\n");
            break;
        case MODE_HOLD:
            if (config.debug.debug_fbw) printf("[modes] exiting hold mode\n");
            break;
    }
    if (imuDataSafe) {
        error_clear(ERROR_IMU, false);
        switch (newMode) {
            case MODE_DIRECT:
                if (config.debug.debug_fbw) printf("[modes] entering direct mode\n");
                currentMode = MODE_DIRECT;
                break;
            case MODE_NORMAL:
                // Automatically enter tune mode if necessary
                if (!mode_tuneisCalibrated()) {
                    toMode(MODE_TUNE);
                    return;
                }
                if (config.debug.debug_fbw) printf("[modes] entering normal mode\n");
                mode_normalInit();
                currentMode = MODE_NORMAL;
                break;
            case MODE_AUTO:
                if (!mode_tuneisCalibrated()) {
                    toMode(MODE_TUNE);
                    return;
                }
                if (config.sensors.gpsEnabled) {
                    // TODO: have a way for auto mode to re-engage if the gps becomes safe again; this is usually due to bad DOP which fixes itself over time
                    if (gpsDataSafe) {
                        // Check to see if we have to calibrate the GPS alt offset
                        if (wifly_getNumGPSSamples() > 0) {
                            gps_calibrateAltOffset(wifly_getNumGPSSamples());
                        }
                        if (config.debug.debug_fbw) printf("[modes] entering auto mode\n");
                        if (mode_autoInit()) {
                            currentMode = MODE_AUTO;
                        } else {
                            toMode(MODE_NORMAL);
                            return;
                        }
                    } else {
                        // GPS is required to be safe for auto and hold modes, fallback to normal mode
                        toMode(MODE_NORMAL);
                        return;
                    }
                } else {
                    toMode(MODE_NORMAL);
                    return;
                }
                break;
            case MODE_TUNE:
                if (!mode_tuneisCalibrated()) {
                    if (config.debug.debug_fbw) printf("[modes] entering tune mode\n");
                    currentMode = MODE_TUNE;
                } else {
                    toMode(MODE_NORMAL);
                    return;
                }
                break;
            case MODE_HOLD:
                if (gpsDataSafe) {
                    if (config.debug.debug_fbw) printf("[modes] entering hold mode\n");
                    currentMode = MODE_HOLD;
                } else {
                    toMode(MODE_NORMAL);
                    return;
                }
                break;
        }
    } else {
        if (config.debug.debug_fbw) printf("[modes] entering direct mode\n");
        error_throw(ERROR_IMU, ERROR_LEVEL_ERR, 250, 0, true, "Entering direct mode due to an IMU failure!");
        currentMode = MODE_DIRECT;
    }
}

void modeRuntime() {
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

Mode getCurrentMode() { return currentMode; }

void setIMUSafe(bool state) {
    if (state != imuDataSafe) {
        imuDataSafe = state;
        if (state) {
            if (config.debug.debug_fbw) printf("[modes] IMU set as safe\n");
        } else {
            toMode(MODE_DIRECT); // Automatically de-init IMU if IMU is deemed unsafe
            if (config.debug.debug_fbw) printf("[modes] IMU set as unsafe\n");
        }
    }
}

void setGPSSafe(bool state) {
    if (state != gpsDataSafe) {
        gpsDataSafe = state;
        if (state) {
            if (config.debug.debug_fbw) printf("[modes] GPS set as safe\n");
            error_clear(ERROR_GPS, false); // Clear any GPS errors
        } else {
            if (config.debug.debug_fbw) printf("[modes] GPS set as unsafe\n");
            if (currentMode == MODE_AUTO || currentMode == MODE_HOLD) {
                toMode(MODE_NORMAL); // Return to normal mode if GPS is deemed unsafe in Auto or Hold modes (require GPS)
            }
        }
    }
}
