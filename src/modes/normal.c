/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include "platform/helpers.h"
#include "platform/time.h"

#include "ctrl/aircraft.h"
#include "ctrl/control.h"
#include "ctrl/flight.h"
#include "ctrl/throttle.h"
#include "io/imu.h"
#include "io/receiver.h"
#include "io/servo.h"
#include "sys/configuration.h"

#include "normal.h"

// The rate at which the roll setpoint returns to its limit from the hold limit
// This is NOT in deg/s, think of it as someone holding the stick at the magnitude of this value
#define CONTROL_ROLL_RETURN 30.f
// The threshold for how close to zero the roll must be to be considered zero (and auto-return to actual zero)
#define CONTROL_ROLL_NEARZERO_THRESHOLD 2.f
// `lerp()` `t` coefficient for auto-return to zero behavior
#define CONTROL_ROLL_RETURN_SMOOTHING 6.f

static f32 rollInput, pitchInput, yawInput;
static f32 rollSet, pitchSet, throttleSet;

static bool overrideYaw = false;
static bool overrideSetpoints = false;

void normal_init() {
    flight_init();
    throttle_init();
    throttle_set_mode(THRMODE_THRUST);
    // Seed setpoints with current angles to avoid a sudden jerk during engagement
    rollSet = imu.roll;
    pitchSet = imu.pitch;
}

void normal_update() {
    // Refresh input data from rx
    rollInput = control_apply_expo(receiver_get((i16)config.pins.inputAil, RECEIVER_MODE_DEGREE)) - 90.f;
    pitchInput = control_apply_expo(receiver_get((i16)config.pins.inputEle, RECEIVER_MODE_DEGREE)) - 90.f;
    if (receiver_has_rud()) {
        yawInput = control_apply_expo(receiver_get((i16)config.pins.inputRud, RECEIVER_MODE_DEGREE)) - 90.f;
    }
    throttleSet = receiver_get((i16)config.pins.inputThrottle, RECEIVER_MODE_PERCENT);

    // If the roll value is above the roll limit, we allow setting up to to the hold limit (enforced later),
    // but constant input is required
    f32 requiredInput = fabsf(rollSet) - config.control.rollLimit;
    // If there is no constant input, we need to slowly bring back roll to the non-hold limit
    if (fabsf(rollSet) > config.control.rollLimit && fabsf(rollInput) < requiredInput) {
        // Override the user's input to bring it back in the opposite direction at the return rate
        rollInput = rollSet < 0 ? CONTROL_ROLL_RETURN : -CONTROL_ROLL_RETURN;
    }
    // Also, if the roll value is near zero and we don't have any input, gradually bring it back to zero
    else if (fabsf(rollSet) < CONTROL_ROLL_NEARZERO_THRESHOLD && fabsf(rollInput) < config.control.controlDeadband) {
        rollInput = lerp(rollSet, 0, CONTROL_ROLL_RETURN_SMOOTHING);
        // Close enough/too small of an output, we can snap cleanly without much jerk
        if (fabsf(rollSet) < 0.1f || rollInput < config.control.controlDeadband) {
            rollSet = 0.f;
        }
    }

    // Calculate control adjustments based on input
    f32 rollAdj = control_calc_adjust(AXIS_ROLL, rollInput, pitchInput);
    f32 pitchAdj = control_calc_adjust(AXIS_PITCH, rollInput, pitchInput);

    // Check for manual overrides of externally set (by API) setpoints
    if (USER_INPUTTING()) {
        overrideSetpoints = false;
    }

    if (!overrideSetpoints) {
        // Use the inputs from the receiver to calculate the setpoint values
        // Take deadband into account to avoid noise from hardware fluctuations
        if (fabsf(rollInput) > config.control.controlDeadband) {
            rollSet += rollAdj;
        }
        if (fabsf(pitchInput) > config.control.controlDeadband) {
            pitchSet += pitchAdj;
        }

        // Enforce bank/pitch protections
        if (fabsf(rollSet) > config.control.rollLimit) {
            if (rollSet > config.control.rollLimitHold) {
                rollSet = config.control.rollLimitHold;
            } else if (rollSet < -config.control.rollLimitHold) {
                rollSet = -config.control.rollLimitHold;
            }
        }
        if (pitchSet > config.control.pitchUpperLimit || pitchSet < config.control.pitchLowerLimit) {
            // Pitch is simply limited to the unsafe thresholds
            if (pitchSet > config.control.pitchUpperLimit) {
                pitchSet = config.control.pitchUpperLimit;
            } else if (pitchSet < config.control.pitchLowerLimit) {
                pitchSet = config.control.pitchLowerLimit;
            }
        }
    }

    // Yaw deadband calculation
    // If we detect any rudder input whatsoever, we wil override what PID wants with the user input
    if (fabsf(yawInput) > config.control.controlDeadband) {
        overrideYaw = true;
    } else {
        overrideYaw = false;
    }

    // Update the flight and throttle systems with calculated setpoints
    flight_update((f64)rollSet, (f64)pitchSet, (f64)yawInput, overrideYaw);
    throttle_set_target(throttleSet);
    throttle_update();
}

void normal_deinit() {
    rollSet = 0.f;
    pitchSet = 0.f;
    overrideYaw = false;
    control_reset();
}

void normal_get(f32 *roll, f32 *pitch) {
    *roll = rollSet;
    *pitch = pitchSet;
}

bool normal_set(f32 roll, f32 pitch, f32 yaw, f32 throttle) {
    // Ensure there are no manual control inputs before we allow setpoints to be externally set
    if (USER_INPUTTING()) {
        return false;
    }
    rollSet = roll;
    pitchSet = pitch;
    yawInput = yaw;
    throttleSet = throttle;
    overrideSetpoints = true;
    return true;
}
