/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>

#include "../io/gps.h"
#include "../io/imu.h"
#include "../io/led.h"
#include "../io/servo.h"

#include "auto.h"
#include "direct.h"
#include "hold.h"
#include "normal.h"
#include "tune.h"

#include "../config.h"

#include "modes.h"

static Mode currentMode = DIRECT;
static bool imuDataSafe = false;
static bool gpsDataSafe = false;

void toMode(Mode newMode) {
    // Run deinit code for currentMode and then run init code for newMode
    switch (currentMode) {
        case DIRECT:
            FBW_DEBUG_printf("[modes] exiting direct mode\n");
            break;
        case NORMAL:
            FBW_DEBUG_printf("[modes] exiting normal mode\n");
            mode_normalDeinit();
            break;
        case AUTO:
            FBW_DEBUG_printf("[modes] exiting auto mode\n");
            break;
        case TUNE:
            FBW_DEBUG_printf("[modes] exiting tune mode\n");
            break;
        case HOLD:
            FBW_DEBUG_printf("[modes] exiting hold mode\n");
            break;
    }
    if (imuDataSafe) {
        led_stop();
        switch (newMode) {
            case DIRECT:
                FBW_DEBUG_printf("[modes] entering direct mode\n");
                currentMode = DIRECT;
                break;
            case NORMAL:
                // Automatically enter tune mode if necessary
                #ifdef PID_AUTOTUNE
                    if (!mode_tuneCheckCalibration()) {
                        toMode(TUNE);
                        return;
                    }
                #else
                    FBW_DEBUG_printf("[modes] entering normal mode\n");
                    mode_normalInit();
                    currentMode = NORMAL;
                #endif
                break;
            case AUTO:
                #ifdef PID_AUTOTUNE
                    if (!mode_tuneCheckCalibration()) {
                        toMode(TUNE);
                        return;
                    }
                #else
                    #ifdef GPS_ENABLED
                        if (gpsDataSafe) {
                            FBW_DEBUG_printf("[modes] entering auto mode\n");
                            if (mode_autoInit()) {
                                currentMode = AUTO;
                            } else {
                                toMode(NORMAL);
                                return;
                            }
                        } else {
                            // GPS is required for auto and hold modes, fallback to normal mode
                            toMode(NORMAL);
                            return;
                        }
                    #else
                        // GPS is required to run auto mode, fallback
                        toMode(NORMAL);
                        return;
                    #endif
                #endif
                break;
            case TUNE:
                if (!mode_tuneCheckCalibration()) {
                    FBW_DEBUG_printf("[modes] entering tune mode\n");
                    currentMode = TUNE;
                } else {
                    toMode(NORMAL);
                    return;
                }
                break;
            case HOLD:
                if (gpsDataSafe) {
                    FBW_DEBUG_printf("[modes] entering hold mode\n");
                    currentMode = HOLD;
                } else {
                    toMode(NORMAL);
                    return;
                }
                break;
        }
    } else {
        // If the IMU is unsafe we only have one option...direct mode
        // Trigger FBW-250 because we are entering direct mode due to an IMU failure
        FBW_DEBUG_printf("[modes] ERROR: [FBW-250] entering direct mode due to IMU failure\n");
        led_blink(250, 0);
        currentMode = DIRECT;
    }
}

void modeRuntime() {
    switch(currentMode) {
        case DIRECT:
            mode_direct();
            break;   
        case NORMAL:
            mode_normal();
            break; 
        case AUTO:
            #ifdef GPS_ENABLED
                mode_auto();
            #endif
            break;
        case TUNE:
            #ifdef PID_AUTOTUNE
                mode_tune();
            #endif
            break;
        case HOLD:
            #ifdef GPS_ENABLED
                mode_hold();
            #endif
            break;
    }
}

uint8_t getCurrentMode() { return currentMode; }

void setIMUSafe(bool state) {
    if (state != imuDataSafe) {
        #ifdef FBW_DEBUG
            if (state) {
                FBW_DEBUG_printf("[modes] IMU set as safe\n");
            } else {
                FBW_DEBUG_printf("[modes] IMU set as unsafe\n");
            }
        #endif
        imuDataSafe = state;
        // Automatically de-init IMU and enable direct mode if IMU is deemed unsafe
        if (!state) {
            toMode(DIRECT);
        }
    }
}

void setGPSSafe(bool state) {
    if (state != gpsDataSafe) {
        #ifdef FBW_DEBUG
            if (state) {
                FBW_DEBUG_printf("[modes] GPS set as safe\n");
            } else {
                FBW_DEBUG_printf("[modes] GPS set as unsafe\n");
            }
        #endif
        gpsDataSafe = state;
        if (!state) {
            if (currentMode == AUTO || currentMode == HOLD) {
                toMode(NORMAL);
            }
        }
    }
}
