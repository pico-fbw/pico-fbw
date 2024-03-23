/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include "platform/int.h"
#include "platform/time.h"

#include "io/esc.h"
#include "io/receiver.h"
#include "io/servo.h"

#include "modes/flight.h"

#include "sys/configuration.h"
#include "sys/control.h"
#include "sys/throttle.h"

#include "normal.h"

// The rate at which the roll setpoint returns to its limit from the hold limit (in deg)
// This is NOT in deg/s, think of it as someone holding the stick at the magnitude of this value
#define CONTROL_ROLL_RETURN_DPS 45.f

#define rolling() (fabsf(rollInput) > config.control[CONTROL_DEADBAND])

#define pitching() (fabsf(pitchInput) > config.control[CONTROL_DEADBAND])

#define yawing() (fabsf(yawInput) > config.control[CONTROL_DEADBAND])

static float rollInput, pitchInput, yawInput;
static float rollSet, pitchSet, throttleSet;

static bool overrideYaw = false;
static bool overrideSetpoints = false;

void normal_init() {
    flight_init();
    throttle.init();
    throttle.mode = THRMODE_THRUST;
}

void normal_update() {
    // Refresh input data from rx
    rollInput = receiver_get((u32)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE) - 90.f;
    pitchInput = receiver_get((u32)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE) - 90.f;
    if (receiver_has_rud())
        yawInput = receiver_get((u32)config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE) - 90.f;
    throttleSet = receiver_get((u32)config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT);

    // If the roll value is above the limit, we do allow setting up to to the hold limit but constant input is required for
    // that, so if we don't have it, bring it back to the hold limit at the specified rate
    if (fabsf(rollSet) > config.control[CONTROL_ROLL_LIMIT] && fabsf(rollInput) < fabsf(rollSet))
        // Override the input to bring it back in the opposite direction at the specified rate
        rollInput = rollSet < 0 ? CONTROL_ROLL_RETURN_DPS : -CONTROL_ROLL_RETURN_DPS;
    // Calculate control adjustments based on input
    float rollAdj = control_calc_adjust(ROLL, rollInput, pitchInput);
    float pitchAdj = control_calc_adjust(PITCH, rollInput, pitchInput);

    // Check for manual overrides of externally set (by API) setpoints
    if (rolling() || pitching() || yawing())
        overrideSetpoints = false;

    if (!overrideSetpoints) {
        // Use the inputs from the receiver to calculate the setpoint values
        // Take deadband into account so we don't get crazy setpoints due to PWM fluctuations
        if (rolling())
            rollSet += rollAdj;
        if (pitching())
            pitchSet += pitchAdj;

        // Make sure the setpoints aren't set to unsafe values so we don't get weird outputs from PID,
        // this is also where our bank/pitch protections come in.
        if (fabsf(rollSet) > config.control[CONTROL_ROLL_LIMIT]) {
            if (rollSet > config.control[CONTROL_ROLL_LIMIT_HOLD]) {
                rollSet = config.control[CONTROL_ROLL_LIMIT_HOLD];
            } else if (rollSet < -config.control[CONTROL_ROLL_LIMIT_HOLD]) {
                rollSet = -config.control[CONTROL_ROLL_LIMIT_HOLD];
            }
        }
        if (pitchSet > config.control[CONTROL_PITCH_UPPER_LIMIT] || pitchSet < config.control[CONTROL_PITCH_LOWER_LIMIT]) {
            // Pitch is simply limited to the unsafe thresholds
            if (pitchSet > config.control[CONTROL_PITCH_UPPER_LIMIT]) {
                pitchSet = config.control[CONTROL_PITCH_UPPER_LIMIT];
            } else if (pitchSet < config.control[CONTROL_PITCH_LOWER_LIMIT]) {
                pitchSet = config.control[CONTROL_PITCH_LOWER_LIMIT];
            }
        }
    }

    // Yaw deadband calculation--if we detect any aileron input whatsoever, we wil override what PID wants with the user input
    if (yawInput > config.control[CONTROL_DEADBAND] || yawInput < -config.control[CONTROL_DEADBAND]) {
        overrideYaw = true;
    } else
        overrideYaw = false;

    // Update the flight and throttle systems with calculated setpoints
    flight_update((double)rollSet, (double)pitchSet, (double)yawInput, overrideYaw);
    throttle.target = throttleSet;
    throttle.update();
}

void normal_deinit() {
    rollSet = 0.f;
    pitchSet = 0.f;
    overrideYaw = false;
    control_reset();
}

bool normal_set(float roll, float pitch, float yaw, float throttle, bool useThrottle) {
    // Ensure there are no manual control inputs before we allow setpoints to be externally set
    if (rolling() || pitching() || yawing())
        return false;
    rollSet = roll;
    pitchSet = pitch;
    yawInput = yaw;
    if (useThrottle)
        throttleSet = throttle;
    overrideSetpoints = true;
    return true;
}
