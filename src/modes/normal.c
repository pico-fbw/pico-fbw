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

static float rollInput, pitchInput, yawInput;
static float rollSet, pitchSet, throttleSet;

static bool overrideYaw = false;
static bool overrideSetpoints = false;

static inline bool rolling() {
    return fabsf(rollInput) > config.control[CONTROL_DEADBAND];
}

static inline bool pitching() {
    return fabsf(pitchInput) > config.control[CONTROL_DEADBAND];
}

static inline bool yawing() {
    return fabsf(yawInput) > config.control[CONTROL_DEADBAND];
}

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
    // This comment is a tribute to the world's stupidest bug where the above code was set to READ from the SERVOS
    // and it took me much longer than I'm willing to admit to find (cue the facepalms, I know ._.)

    // Check for manual overrides of externally set (by API) setpoints
    if (rolling() || pitching() || yawing())
        overrideSetpoints = false;

    if (!overrideSetpoints) {
        // Use the inputs from the receiver to calculate the setpoint values
        // Take deadband into account so we don't get crazy setpoints due to PWM fluctuations
        if (rolling())
            rollSet += control_calc_adjust(ROLL, rollInput, pitchInput);
        if (pitching())
            pitchSet += control_calc_adjust(PITCH, rollInput, pitchInput);

        // Make sure the setpoints aren't set to unsafe values so we don't get weird outputs from PID,
        // this is also where our bank/pitch protections come in.
        if (fabsf(rollSet) > config.control[CONTROL_ROLL_LIMIT]) {
            // If the roll values are unsafe, we do allow setting up to to the hold limit but constant input is required, so
            // check for that
            if (fabsf(rollInput) < fabsf(rollSet)) {
                if (rollSet > 0) {
                    rollSet -= 0.05f;
                } else if (rollSet < 0) {
                    rollSet += 0.05f;
                }
            }
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
    rollSet = 0.0f;
    pitchSet = 0.0f;
    overrideYaw = false;
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
