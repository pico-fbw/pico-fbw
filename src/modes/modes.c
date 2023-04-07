#include "pico/stdlib.h"

#include "../io/imu.h"
#include "../io/servo.h"
#include "../io/led.h"
#include "../config.h"

#include "modes.h"

/* Define local variables--these should not be accessable to other files. */

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

void setMode(uint mode) {
    // Any code that needs to run on the switching of modes goes here--
    if (mode == 0) {
        // Set the current working mode
        cmode = mode;
        // Mode has been set to direct, quick blink LED (IMU error after startup)
        led_blink(250);
        /*
        THIS METHOD IS UNUSED, I2C protocol freezes up after a disconnection and I couldn't find a solution, so it is disabled for now

        // Set up interrupt to try and reconnect to IMU every 5s
        add_repeating_timer_ms(5000, imuReconnect, NULL, &imuTimer);
        */
    } else if (mode == 1) {
        // Make sure it is okay to set to normal mode
        if (imuDataSafe) {
            cmode = mode;
            // Mode has been set to normal
            led_blink_stop();
        }
    }
}
uint getMode() {
    return cmode;
}

void setIMUSafe(int state) {
    imuDataSafe = state;
    // Automatically de-init i2c and set into normal mode if IMU is deemed unsafe
    if (!state) {
        imu_deinit();
        setMode(0);
    }
}