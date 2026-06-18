/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

// TODO: refactor, make logic less spaghetti and convoluted

#include <math.h>
#include "platform/time.h"

#include "ctrl/aircraft.h"
#include "ctrl/control.h"
#include "ctrl/flight.h"
#include "ctrl/throttle.h"
#include "io/receiver.h"
#include "io/servo.h"
#include "sys/configuration.h"

#include "normal.h"

// The rate at which the roll setpoint returns to its limit from the hold limit (in deg)
// This is NOT in deg/s, think of it as someone holding the stick at the magnitude of this value
#define CONTROL_ROLL_RETURN_DPS 45.f
// The threshold for how close to zero the roll must be to be considered zero (and auto-return to actual zero)
#define CONTROL_ROLL_NEARZERO_THRESHOLD 1.f

static f32 rollInput, pitchInput, yawInput;
static f32 rollSet, pitchSet, throttleSet;

static bool overrideYaw = false;
static bool overrideSetpoints = false;

void normal_init() {
    flight_init();
    throttle_init();
    throttle_set_mode(THRMODE_THRUST);
}

void normal_update() {
    // Refresh input data from rx
    rollInput = receiver_get((i16)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE) - 90.f;
    pitchInput = receiver_get((i16)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE) - 90.f;
    if (receiver_has_rud()) {
        yawInput = receiver_get((i16)config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE) - 90.f;
    }
    throttleSet = receiver_get((i16)config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT);

    // If the roll value is above the limit, we do allow setting up to to the hold limit but constant input is required
    // for that, so if we don't have it, bring it back to the hold limit at the specified rate
    if (fabsf(rollSet) > config.control[CONTROL_ROLL_LIMIT] && fabsf(rollInput) < fabsf(rollSet)) {
        // Override the input to bring it back in the opposite direction at the specified rate
        rollInput = rollSet < 0 ? CONTROL_ROLL_RETURN_DPS : -CONTROL_ROLL_RETURN_DPS;
    }
    // Also, if the roll value is near zero and we don't have any input, bring it back to zero automatically to level
    if (fabsf(rollSet) < CONTROL_ROLL_NEARZERO_THRESHOLD && fabsf(rollInput) < config.control[CONTROL_DEADBAND]) {
        rollSet = 0.f;
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
        // Take deadband into account so we don't get crazy setpoints due to PWM fluctuations
        if (ROLL_INPUT()) {
            rollSet += rollAdj;
        }
        if (PITCH_INPUT()) {
            pitchSet += pitchAdj;
        }

        // Make sure the setpoints aren't set to unsafe values so we don't get weird outputs from PID,
        // this is also where our bank/pitch protections come in.
        if (fabsf(rollSet) > config.control[CONTROL_ROLL_LIMIT]) {
            if (rollSet > config.control[CONTROL_ROLL_LIMIT_HOLD]) {
                rollSet = config.control[CONTROL_ROLL_LIMIT_HOLD];
            } else if (rollSet < -config.control[CONTROL_ROLL_LIMIT_HOLD]) {
                rollSet = -config.control[CONTROL_ROLL_LIMIT_HOLD];
            }
        }
        if (pitchSet > config.control[CONTROL_PITCH_UPPER_LIMIT] ||
            pitchSet < config.control[CONTROL_PITCH_LOWER_LIMIT]) {
            // Pitch is simply limited to the unsafe thresholds
            if (pitchSet > config.control[CONTROL_PITCH_UPPER_LIMIT]) {
                pitchSet = config.control[CONTROL_PITCH_UPPER_LIMIT];
            } else if (pitchSet < config.control[CONTROL_PITCH_LOWER_LIMIT]) {
                pitchSet = config.control[CONTROL_PITCH_LOWER_LIMIT];
            }
        }
    }

    // Yaw deadband calculation
    // If we detect any rudder input whatsoever, we wil override what PID wants with the user input
    if (fabsf(yawInput) > config.control[CONTROL_DEADBAND]) {
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
