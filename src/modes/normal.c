/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "../io/flash.h"
#include "../io/imu.h"
#include "../io/pwm.h"
#include "../io/servo.h"

#include "modes.h"
#include "auto.h"
#include "tune.h"
#include "flight.h"

#include "normal.h"

static float rollInput;
static float pitchInput;
static float yawInput;

static float rollSet;
static float pitchSet;

static bool overrideYaw = false;

static bool overrideSetpoints = false;

void mode_normalInit() {
    flight_init(); // Initialize baseline flight system
}

void mode_normal() {
    // Refresh input data from rx
    rollInput = pwm_read((uint)flash.pins[PINS_SERVO_AIL], PWM_MODE_DEG) - 90;
    pitchInput = pwm_read((uint)flash.pins[PINS_SERVO_ELEV], PWM_MODE_DEG) - 90;
    if ((ControlMode)flash.general[GENERAL_CONTROL_MODE] == CTRLMODE_3AXIS_ATHR ||
        (ControlMode)flash.general[GENERAL_CONTROL_MODE] == CTRLMODE_3AXIS) {
        yawInput = pwm_read((uint)flash.pins[PINS_SERVO_RUD], PWM_MODE_DEG) - 90;
    }
    
    // Check for manual overrides of externally set setpoints
    if (fabsf(rollInput) > flash.control[CONTROL_DEADBAND] ||
    fabsf(pitchInput) > flash.control[CONTROL_DEADBAND] ||
    fabsf(yawInput) > flash.control[CONTROL_DEADBAND]) {
        overrideSetpoints = false;
    }
    if (!overrideSetpoints) {
        // Use the rx inputs to set the setpoint control values
        // Deadband calculations so we don't get crazy values due to PWM fluctuations
        if (fabsf(rollInput) > flash.control[CONTROL_DEADBAND]) {
            // If the input is not within the deadband, add the smoothed input value on top of the current setpoint
            // We must smooth the value because this calculation is done many times per second, so no smoothing would result
            // in extremely (and I do really mean extreme) touchy controls.
            rollSet += rollInput * flash.control[CONTROL_SENSITIVITY];
        }
        if (fabsf(pitchInput) > flash.control[CONTROL_DEADBAND]) {
            pitchSet += pitchInput * flash.control[CONTROL_SENSITIVITY];
        }

        // Make sure the PID setpoints aren't set to unsafe values so we don't get weird outputs from PID,
        // this is also where our bank/pitch protections come in.
        if (fabsf(rollSet) > flash.control[CONTROL_ROLL_LIMIT]) {
            // If the roll values are unsafe, we do allow setting up to 67 but constant input is required, so check for that
            if (!(abs(rollInput) >= abs(rollSet))) {
                if (rollSet > 0) {
                    rollSet -= 0.05;
                } else if (rollSet < 0) {
                    rollSet += 0.05;
                }
            }
            if (rollSet > flash.control[CONTROL_ROLL_LIMIT_HOLD]) {
                rollSet = flash.control[CONTROL_ROLL_LIMIT_HOLD];
            } else if (rollSet < -flash.control[CONTROL_ROLL_LIMIT_HOLD]) {
                rollSet = -flash.control[CONTROL_ROLL_LIMIT_HOLD];
            }
        }
        if (pitchSet >flash.control[CONTROL_PITCH_UPPER_LIMIT] || pitchSet < flash.control[CONTROL_PITCH_LOWER_LIMIT]) {
            // Pitch is simply limited to the unsafe thresholds
            if (pitchSet >flash.control[CONTROL_PITCH_UPPER_LIMIT]) {
                pitchSet =flash.control[CONTROL_PITCH_UPPER_LIMIT];
            } else if (pitchSet < flash.control[CONTROL_PITCH_LOWER_LIMIT]) {
                pitchSet = flash.control[CONTROL_PITCH_LOWER_LIMIT];
            }
        }
    }

    // Yaw deadband calculation--if we detect any aileron input whatsoever, we wil override what PID wants with the user input
    if (yawInput > flash.control[CONTROL_DEADBAND] || yawInput < -flash.control[CONTROL_DEADBAND]) {
        overrideYaw = true;
    } else {
        overrideYaw = false;
    }

    // Update the flight system with calculated setpoints
    flight_update((double)rollSet, (double)pitchSet, (double)yawInput, overrideYaw);
}

void mode_normalDeinit() {
    rollSet = 0.0;
    pitchSet = 0.0;
    overrideYaw = false;
}

bool mode_normalSetSetpoints(float roll, float pitch, float yaw) {
    // Ensure there are no manual control inputs before we allow setpoints to be externally set
    if (rollInput < flash.control[CONTROL_DEADBAND] && rollInput > -flash.control[CONTROL_DEADBAND] &&
    pitchInput < flash.control[CONTROL_DEADBAND] && pitchInput > -flash.control[CONTROL_DEADBAND] &&
    yawInput < flash.control[CONTROL_DEADBAND] && yawInput > -flash.control[CONTROL_DEADBAND]) {
        return false;
    } else {
        rollSet = roll;
        pitchSet = pitch;
        yawInput = yaw;
        overrideSetpoints = true;
        return true;
    }
}
