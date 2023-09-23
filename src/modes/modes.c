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

#include "../config.h"

#include "modes.h"

static Mode currentMode = MODE_DIRECT;
static bool imuDataSafe = false;
static bool gpsDataSafe = false;

void toMode(Mode newMode) {
    // Run deinit code for currentMode and then run init code for newMode
    switch (currentMode) {
        case MODE_DIRECT:
            FBW_DEBUG_printf("[modes] exiting direct mode\n");
            break;
        case MODE_NORMAL:
            FBW_DEBUG_printf("[modes] exiting normal mode\n");
            mode_normalDeinit();
            break;
        case MODE_AUTO:
            FBW_DEBUG_printf("[modes] exiting auto mode\n");
            break;
        case MODE_TUNE:
            FBW_DEBUG_printf("[modes] exiting tune mode\n");
            break;
        case MODE_HOLD:
            FBW_DEBUG_printf("[modes] exiting hold mode\n");
            break;
    }
    if (imuDataSafe) {
        error_clear(ERROR_IMU, false);
        switch (newMode) {
            case MODE_DIRECT:
                FBW_DEBUG_printf("[modes] entering direct mode\n");
                currentMode = MODE_DIRECT;
                break;
            case MODE_NORMAL:
                // Automatically enter tune mode if necessary
                #ifdef PID_MODE_AUTOMODE_TUNE
                    if (!mode_tuneisCalibrated()) {
                        toMode(MODE_TUNE);
                        return;
                    }
                #else
                    FBW_DEBUG_printf("[modes] entering normal mode\n");
                    mode_normalInit();
                    currentMode = MODE_NORMAL;
                #endif
                break;
            case MODE_AUTO:
                #ifdef PID_MODE_AUTOMODE_TUNE
                    if (!mode_tuneisCalibrated()) {
                        toMode(MODE_TUNE);
                        return;
                    }
                #else
                    #ifdef GPS_ENABLED
                        // TODO: have a way for auto mode to re-engage if the gps becomes safe again; this is usually due to bad DOP which fixes itself over time
                        if (gpsDataSafe) {
                            // Check to see if we have to calibrate the GPS alt offset
                            if (wifly_getNumGPSSamples() > 0) {
                                gps_calibrateAltOffset(wifly_getNumGPSSamples());
                            }
                            FBW_DEBUG_printf("[modes] entering auto mode\n");
                            if (mode_autoInit()) {
                                currentMode = MODE_AUTO;
                            } else {
                                toMode(MODE_NORMAL);
                                return;
                            }
                        } else {
                            // GPS is required for auto and hold modes, fallback to normal mode
                            toMode(MODE_NORMAL);
                            return;
                        }
                    #else
                        // GPS is required to run auto mode, fallback
                        toMode(MODE_NORMAL);
                        return;
                    #endif
                #endif
                break;
            case MODE_TUNE:
                if (!mode_tuneisCalibrated()) {
                    FBW_DEBUG_printf("[modes] entering tune mode\n");
                    currentMode = MODE_TUNE;
                } else {
                    toMode(MODE_NORMAL);
                    return;
                }
                break;
            case MODE_HOLD:
                if (gpsDataSafe) {
                    FBW_DEBUG_printf("[modes] entering hold mode\n");
                    currentMode = MODE_HOLD;
                } else {
                    toMode(MODE_NORMAL);
                    return;
                }
                break;
        }
    } else {
        FBW_DEBUG_printf("[modes] entering direct mode\n");
        // If the IMU is unsafe we only have one option...direct mode
        // Trigger FBW-250 because we are entering direct mode due to an IMU failure
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
            #ifdef GPS_ENABLED
                mode_auto();
            #endif
            break;
        case MODE_TUNE:
            #ifdef PID_MODE_AUTOMODE_TUNE
                mode_tune();
            #endif
            break;
        case MODE_HOLD:
            #ifdef GPS_ENABLED
                mode_hold();
            #endif
            break;
    }
}

Mode getCurrentMode() { return currentMode; }

void setIMUSafe(bool state) {
    if (state != imuDataSafe) {
        imuDataSafe = state;
        if (state) {
            FBW_DEBUG_printf("[modes] IMU set as safe\n");
        } else {
            toMode(MODE_DIRECT); // Automatically de-init IMU if IMU is deemed unsafe
            FBW_DEBUG_printf("[modes] IMU set as unsafe\n");
        }
    }
}

void setGPSSafe(bool state) {
    if (state != gpsDataSafe) {
        gpsDataSafe = state;
        if (state) {
            FBW_DEBUG_printf("[modes] GPS set as safe\n");
            error_clear(ERROR_GPS, false); // Clear any GPS errors
        } else {
            FBW_DEBUG_printf("[modes] GPS set as unsafe\n");
            if (currentMode == MODE_AUTO || currentMode == MODE_HOLD) {
                toMode(MODE_NORMAL); // Return to normal mode if GPS is deemed unsafe in Auto or Hold modes (require GPS)
            }
        }
    }
}
