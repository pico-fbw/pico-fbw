/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>

#include "../io/imu.h"
#include "../io/servo.h"
#include "../io/led.h"

#include "auto.h"
#include "direct.h"
#include "hold.h"
#include "normal.h"
#include "tune.h"

#include "../config.h"

#include "modes.h"

static uint8_t currentMode = DIRECT;
static bool imuDataSafe = false;

void toMode(uint8_t newMode) {
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
        led_blink_stop();
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
                    #ifdef WIFLY_ENABLED
                        FBW_DEBUG_printf("[modes] entering auto mode\n");
                        if (mode_autoInit()) {
                            currentMode = AUTO;
                        } else {
                            toMode(NORMAL);
                            return;
                        }
                    #else
                        // Wi-Fly is required to run auto mode, fallback to normal mode
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
                FBW_DEBUG_printf("[modes] entering hold mode\n");
                currentMode = HOLD;
                break;
        }
    } else {
        // If the IMU is unsafe we only have one option...direct mode
        // Trigger FBW-250 because we are entering direct mode due to an IMU failure
        led_blink(250);
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
            #ifdef WIFLY_ENABLED
                mode_auto();
            #endif
            break;
        case TUNE:
            #ifdef PID_AUTOTUNE
                mode_tune();
            #endif
            break;
        case HOLD:
            #ifdef WIFLY_ENABLED
                mode_hold();
            #endif
            break;
    }
}

void setIMUSafe(bool state) {
    imuDataSafe = state;
    // Automatically de-init i2c and set into direct mode if IMU is deemed unsafe
    if (!state) {
        imu_deinit();
        toMode(DIRECT);
    }
}
