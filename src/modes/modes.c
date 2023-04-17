#include <stdbool.h>
#include "pico/stdlib.h"

#include "../io/imu.h"
#include "../io/servo.h"
#include "../io/led.h"
#include "direct.h"
#include "normal.h"
#include "auto.h"
#include "tune.h"
#include "../config.h"

#include "modes.h"

/* Define local variables--these should not be accessable to other files. */

// Variable to store the system's current mode regardless of the mode currently being requested
// Mode is direct by default until we prove the IMU data is safe
uint cmode = 0;
// Same thing with IMU data itself, bad until proven
bool imuDataSafe = false;

// // Timer var for IMU reconnection
// struct repeating_timer imuTimer;

/* End variable definitions. */

// /**
//  * Attempts to reconnect to the IMU and resume normal mode. Usually called from the timer created when switching to direct mode.
// */
// bool imuReconnect(struct repeating_timer *t) {
//     if (imu_init() == 0) {
//         if (imu_configure() == 0) {
//             // If IMU is able to successfully reconnect, cancel the reconnection timer and resume normal mode
//             cancel_repeating_timer(&imuTimer);
//             setIMUSafe(true);
//             setMode(1);
//         }
//     }
//     return true;
// }

void mode(uint smode) {
    // If the selected mode is the same as the current mode, run the specified mode's cycle function
    switch(smode) {
        case DIRECT:
            // If the current mode is not the selected mode, run the mode switching code for that mode and change the mode
            if (cmode != DIRECT) {
                // Set the current working mode
                cmode = smode;
                // Only set LED as blinking if the IMU has encountered an error, not if the user purposely enters direct mode
                if (!imuDataSafe) {
                    led_blink(250);
                }
                // Reset normal mode, this is just in case we go back to normal mode, we don't want to keep the previous setpoints and have the system snap back to them/
                mode_normalReset();
                
                // THIS FUNCTION IS UNUSED, I2C protocol freezes up after a disconnection and I couldn't find a solution,
                // so it is disabled until I can find a workaround, I do think it is possible.
                // // Set up interrupt to try and reconnect to IMU every 5s
                // add_repeating_timer_ms(5000, imuReconnect, NULL, &imuTimer);
            }
            mode_direct();
        case NORMAL:
            if (cmode != NORMAL) {
                // Make sure it is okay to set to normal mode
                if (imuDataSafe) {
                    cmode = smode;
                    // Mode has been set to normal
                    led_blink_stop();
                } else {
                    // If we are unable to set to normal mode, revert to direct mode
                    mode(0);
                }
            }
            mode_normal();
        case AUTO:
            if (cmode != AUTO) {
                if (imuDataSafe) {
                    cmode = smode;
                    led_blink_stop();
                    // TODO: auto enter tuning mode if PID tuning has not been done yet, do this once I figure out specifics of how that data is stored
                } else {
                    mode(0);
                }
            }
            mode_auto();
        case TUNE:
            mode_tune();    
    }
}

void setIMUSafe(int state) {
    imuDataSafe = state;
    // Automatically de-init i2c and set into direct mode if IMU is deemed unsafe
    if (!state) {
        imu_deinit();
        mode(0);
    }
}
