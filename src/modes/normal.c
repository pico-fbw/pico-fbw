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

inertialAngles angles;

float rollInput;
float pitchInput;
float yawInput;

double rollSet;
double pitchSet;

bool overrideYaw = false;

// Internal function that we will later push to the second core to compute the PID math for all controllers
static inline void normal_computePID() {
    while (true) {
        flight_update(rollSet, angles.roll, pitchSet, angles.pitch, yawInput, angles.yaw, overrideYaw);
    }
}

void mode_normalInit() {
    flight_init();
    multicore_launch_core1(normal_computePID);
}

void mode_normal() {
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
        overrideYaw = true;
    } else {
        overrideYaw = false;
    }
}

void mode_normalDeinit() {
    multicore_reset_core1(); // Reset the second core for use elsewhere
}

void mode_normalReset() {
    rollSet = 0.0;
    pitchSet = 0.0;
    overrideYaw = false;
}
