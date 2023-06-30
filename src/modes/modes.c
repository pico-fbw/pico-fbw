/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>

#include "../io/imu.h"
#include "../io/servo.h"
#include "../io/led.h"
#include "direct.h"
#include "normal.h"
#include "auto.h"
#include "tune.h"

#include "../config.h"

#include "modes.h"

// TODO: fix the weird thing with init/deinit of modes; make that happen in modes.c instead of...all over the place
// also just generally clean up this file lol

// Variable to store the system's current mode regardless of the mode currently being requested
uint8_t cmode = DIRECT; // Mode is direct by default until we prove the IMU data is safe

bool imuDataSafe = false;

void mode(uint8_t smode) {
    // If the selected mode is the same as the current mode, run the specified mode's cycle function
    switch(smode) {
        case DIRECT:
            // If the current mode is not the selected mode, run the mode switching code for that mode and change the mode
            if (cmode != DIRECT) {
                FBW_DEBUG_printf("[modes] entering direct mode \n");
                cmode = DIRECT;
                // Set LED as blinking if the IMU has encountered an error, not if the user purposely enters direct mode
                if (!imuDataSafe) {
                    led_blink(250);
                }
                // Reset normal mode, this is just in case we go back to normal mode, we don't want to keep the previous setpoints and have the system snap back to them/
                mode_normalReset();
            }
            mode_direct();
            break;   
        case NORMAL:
            if (cmode != NORMAL) {
                // Make sure it is okay to set to normal mode
                if (imuDataSafe) {
                    // Enter auto mode if tuning has been done before, otherwise automatically enter tuning mode
                    if (mode_tuneCheckCalibration()) {
                        FBW_DEBUG_printf("[modes] entering normal mode \n");
                        cmode = NORMAL;
                    } else {
                        FBW_DEBUG_printf("[modes] entering tune mode \n");
                        cmode = TUNE;
                    }
                    led_blink_stop();
                    break;
                } else {
                    // If we are unable to set to normal mode, revert to direct mode
                    // We use recursive function calling here so that the direct mode init code runs
                    mode(DIRECT);
                }
            }
            mode_normal();
            break;
        #ifdef WIFLY_ENABLED    
        case AUTO:
            if (cmode != AUTO) {
                if (imuDataSafe) {
                    if (mode_tuneCheckCalibration()) {
                        FBW_DEBUG_printf("[modes] entering auto mode \n");
                        cmode = AUTO;
                    } else {
                        FBW_DEBUG_printf("[modes] entering tune mode \n");
                        cmode = TUNE;
                    }
                    led_blink_stop();
                    break;
                } else {
                    mode(DIRECT);
                }
            }
            mode_auto();
            break;
        #endif    
        #ifdef PID_AUTOTUNE    
        case TUNE:
            // smode might not equal TUNE if we are switching from auto mode so we set it directly
            FBW_DEBUG_printf("[modes] entering tune mode \n");
            cmode = TUNE;
            // This logic is here for safety and also for if tuning mode gets called directly in the case of no mode switching,
            // these checks will pass if it has been automatically entered from auto mode
            if (imuDataSafe) {
                if (!mode_tuneCheckCalibration()) {
                    mode_tune();
                } else {
                    // Recursive function calling so checks can be done
                    mode(NORMAL);
                }
            } else {
                mode(DIRECT);
            }
            break;
        #endif
        case HOLD:
            // TODO
            break;
    }
}

void setIMUSafe(bool state) {
    imuDataSafe = state;
    // Automatically de-init i2c and set into direct mode if IMU is deemed unsafe
    if (!state) {
        imu_deinit();
        mode(DIRECT);
    }
}
