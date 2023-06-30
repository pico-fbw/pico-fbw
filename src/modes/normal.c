#include <stdlib.h>
#include <stdbool.h>
/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include "pico/multicore.h"

#include "../io/imu.h"
#include "../io/pwm.h"
#include "../io/servo.h"
#include "../io/flash.h"
#include "../io/led.h"
#include "../lib/pid.h"

#include "../config.h"

#include "modes.h"
#include "auto.h"
#include "tune.h"
#include "flight.h"

#include "normal.h"

// Keeps the state of if normal mode has been initialized or not and if it is the first time initializing
bool normalInitialized = false;

inertialAngles angles;

float rollInput;
float pitchInput;
float yawInput;

float rollSet;
float pitchSet;
float yawSet;

bool yawdamp_on = false;

// Internal function that we will later push to the second core to compute the PID math for all controllers
static inline void normal_computePID() {
    while (true) {
        flight_update(rollSet, angles.roll, pitchSet, angles.pitch, yawSet, angles.heading, yawdamp_on);
    }
}

static void normalInit() {
    mode_autoDeinit();
    flight_init();
    multicore_launch_core1(normal_computePID);
}

void mode_normalDeinit() {
    // Remove the normal mode initialized flag and reset the second core for use elsewhere
    normalInitialized = false;
    multicore_reset_core1();
}

void mode_normal() {
    // Initialize normal mode if we haven't already
    if (!normalInitialized) {
        normalInit();
        normalInitialized = true;
    }
    // Refresh input data from IMU and rx
    angles = imu_getAngles();
    rollInput = pwm_readDeg(0) - 90;
    pitchInput = pwm_readDeg(1) - 90;
    yawInput = pwm_readDeg(2) - 90;

    if (!flight_checkEnvelope(angles.roll, angles.pitch)) {
        return;
    }

    // Use the rx inputs to set the setpoint control values
    // Deadband calculations so we don't get crazy values due to PWM fluctuations
    if (rollInput > DEADBAND_VALUE || rollInput < -DEADBAND_VALUE) {
        // If the input is not within the deadband, add the smoothed input value on top of the current setpoint
        // We must smooth the value because this calculation is done many times per second, so no smoothing would result
        // in extremely (and I do really mean extreme) touchy controls.
        rollSet += rollInput * SETPOINT_SMOOTHING_VALUE;
    }
    if (pitchInput > DEADBAND_VALUE || pitchInput < -DEADBAND_VALUE) {
        pitchSet += pitchInput * SETPOINT_SMOOTHING_VALUE;
    }

    // Make sure the PID setpoints aren't set to unsafe values so we don't get weird outputs from PID,
    // this is also where our bank/pitch protections come in.
    if (rollSet > ROLL_LIMIT || rollSet < -ROLL_LIMIT) {
        // If the roll values are unsafe, we do allow setting up to 67 but constant input is required, so check for that
        if (!(abs(rollInput) >= abs(rollSet))) {
            if (rollSet > 0) {
                rollSet -= 0.05;
            } else if (rollSet < 0) {
                rollSet += 0.05;
            }
        }
        if (rollSet > ROLL_LIMIT_HOLD) {
            rollSet = ROLL_LIMIT_HOLD;
        } else if (rollSet < -ROLL_LIMIT_HOLD) {
            rollSet = -ROLL_LIMIT_HOLD;
        }
    }
    if (pitchSet > PITCH_UPPER_LIMIT || pitchSet < PITCH_LOWER_LIMIT) {
        // Pitch is simply limited to the unsafe thresholds
        if (pitchSet > PITCH_UPPER_LIMIT) {
            pitchSet = PITCH_UPPER_LIMIT;
        } else if (pitchSet < PITCH_LOWER_LIMIT) {
            pitchSet = PITCH_LOWER_LIMIT;
        }
    }

    // Yaw deadband calculation--if we detect any aileron input whatsoever, we wil override what PID wants with the user input
    if (yawInput > DEADBAND_VALUE || yawInput < -DEADBAND_VALUE) {
        yawSet = yawInput;
        yawdamp_on = false;
    } else {
        // If there is no user input on rudder axis, first check if we are in a turn or not
        // Keep in mind this is different than if the system is making inputs on ailerons! We only want this to activate during turns, not stabilizations.
        if (rollSet > DEADBAND_VALUE || rollSet < -DEADBAND_VALUE) {
            // If we are in a turn, set the yaw damper as disabled and set the yaw value to a reduced version of our aileron value to coordinate our turn
            yawdamp_on = false;
            yawSet = flight_getRollOut() * RUDDER_TURNING_VALUE;
        } else {
            // If the yaw damper is set as off still and we are not in a turn, that means we have just transitioned to this phase, so we should update the yaw setpoint
            if (!yawdamp_on) {
                yawSet = angles.heading;
            }
            yawdamp_on = true;
        }
    }
}

void mode_normalReset() {
    rollSet = 0.0f;
    pitchSet = 0.0f;
    yawSet = 0.0f;
    yawdamp_on = false;
}
