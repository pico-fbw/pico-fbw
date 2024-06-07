/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include "platform/time.h"

#include "io/aahrs.h"
#include "io/receiver.h"

#include "modes/aircraft.h"
#include "modes/flight.h"

#include "sys/configuration.h"
#include "sys/throttle.h"

#include "launch.h"

#define LAUNCH_ACCEL_THRESHOLD 1.2 // Threshold for detecting a launch (g)
#define AUTO_ENGAGE_DELAY_S 5      // Delay between a launch and auto mode engaging

typedef enum LaunchStatus {
    LAUNCH_AWAITING,
    LAUNCH_CLIMBING,
} LaunchStatus;

static Mode afterLaunch = MODE_INVALID; // The mode to enter after an autolaunch takes place
static f32 climbAngle = 0.f;            // The pitch angle to climb away at
static LaunchStatus status = LAUNCH_AWAITING;

/**
 * @return true if a high acceleration above the launch threshold is detected on any axis
 */
static inline bool aahrs_has_launch_accel() {
    for (u32 i = 0; i < 3; i++) {
        if (fabsf(aahrs.accel[i]) > LAUNCH_ACCEL_THRESHOLD)
            return true;
    }
    return false;
}

// Callback to return to the afterLaunch mode.
static i32 return_to_mode(void *data) {
    aircraft.change_to(afterLaunch);
    return 0;
    (void)data;
}

bool launch_init(Mode return_to) {
    afterLaunch = return_to;
    flight_init();
    throttle.init();
    throttle.mode = THRMODE_THRUST;
    // Set idle thrust to indicate that we're ready to launch
    throttle.target = calibration.esc[ESC_DETENT_IDLE];
    return true;
}

void launch_update() {
    switch (status) {
        case LAUNCH_AWAITING:
            climbAngle = aahrs.pitch;
            // Detect if we're launching
            // To do this, we check for an abnormal/sudden increase in acceleration
            // which indicates the user probably threw the aircraft
            if (aahrs_has_launch_accel()) {
                // Launch is happening right now, set max thrust
                throttle.target = calibration.esc[ESC_DETENT_MAX];
                // If we need to return to auto mode after launch, do so after a delay
                if (afterLaunch == MODE_AUTO)
                    callback_in_ms(AUTO_ENGAGE_DELAY_S * 1000, return_to_mode, NULL);
                status = LAUNCH_CLIMBING;
            }
            break;
        case LAUNCH_CLIMBING:
            switch (afterLaunch) {
                case MODE_AUTO:
                    // For auto mode, we've already set a callback to switch into it after some time, so just wait
                    break;
                default:
                    // For all other modes, await user input to switch back
                    if (USER_INPUTTING()) {
                        aircraft.change_to(afterLaunch);
                        return;
                    }
                    break;
            }
    }
    flight_update(0.0, (f64)climbAngle, 0.0, false);
    throttle.update();
}
