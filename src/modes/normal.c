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

#include "normal.h"

// Keeps the state of if normal mode has been initialized or not and if it is the first time initializing
bool normalInitialized = false;
bool normal_firstTimeInit = true;

inertialAngles angles;
PIDController rollPID;
PIDController pitchPID;
PIDController yawPID;

float rollAngle;
float pitchAngle;
float yawAngle;
float rollIn;
float pitchIn;
float yawIn;

float rollSetpoint;
float pitchSetpoint;
float yawSetpoint;

bool yawdamp_on = false;
float yawOut;


// Internal function that we will later push to the second core to compute the PID math for all controllers
static void normal_computePID() {
    while (true) {
        pid_update(&rollPID, rollSetpoint, rollAngle);
        pid_update(&pitchPID, pitchSetpoint, pitchAngle);
        pid_update(&yawPID, yawSetpoint, yawAngle);
    }
}

bool normalInit() {
    mode_autoDeinit();
    #ifdef PID_AUTOTUNE
        // If autotuning is enabled, first make sure it's been completed before we allow normal mode
        if (mode_tuneCheckCalibration()) {
            // Now pull in constants from autotuning
            rollPID = (PIDController){flash_read(1, 1), flash_read(1, 2), flash_read(1, 3), roll_tau, -AIL_LIMIT, AIL_LIMIT, roll_integMin, roll_integMax, roll_kT};
            pitchPID = (PIDController){flash_read(2, 1), flash_read(2, 2), flash_read(2, 3), pitch_tau, -ELEV_LIMIT, ELEV_LIMIT, pitch_integMin, pitch_integMax, pitch_kT};
        } else {
            // The firstTimeInit variable is here so that we only call the led_blink function once, otherwise the LED simply won't change
            if (normal_firstTimeInit) {
                led_blink(2000);
                normal_firstTimeInit = false;
            }
            // Revert to direct
            mode(DIRECT);
            return false;
        }
    #else
        // If autotuning is disabled, pull in constants from manual tuning
        rollPID = (PIDController){roll_kP, roll_kI, roll_kD, roll_tau, -AIL_LIMIT, AIL_LIMIT, roll_integMin, roll_integMax, roll_kT};
        pitchPID = (PIDController){pitch_kP, pitch_kI, pitch_kD, pitch_tau, -ELEV_LIMIT, ELEV_LIMIT, pitch_integMin, pitch_integMax, pitch_kT};
    #endif
    yawPID = (PIDController){yaw_kP, yaw_kI, yaw_kD, yaw_tau, -RUD_LIMIT, RUD_LIMIT, yaw_integMin, yaw_integMax, yaw_kT};
    // Set up PID controllers
    pid_init(&rollPID);
    pid_init(&pitchPID);
    pid_init(&yawPID);
    // Wake the second core and tell it to compute PID values, that's all it will be doing
    multicore_launch_core1(normal_computePID);
    return true;
}

void mode_normalDeinit() {
    // Remove the normal mode initialized flag and reset the second core for use elsewhere
    normalInitialized = false;
    multicore_reset_core1();
}

void mode_normal() {
    // Initialize normal mode if we haven't already
    if (!normalInitialized) {
        if (normalInit()) {
            normalInitialized = true;
        } else {
            return;
        }
    }
    // Refresh input data from IMU and rx
    angles = imu_getAngles();
    rollAngle = angles.roll;
    pitchAngle = angles.pitch;
    yawAngle = angles.heading;
    // Convert the yaw angle to a value from -180 to 180 like all other values instead of 0 to 359 (heading)
    if (yawAngle >= 180) {
        yawAngle -= 360;
    }
    rollIn = pwm_readDeg(0) - 90;
    pitchIn = pwm_readDeg(1) - 90;
    yawIn = pwm_readDeg(2) - 90;

    // Failsafe to check if actual IMU values are too out of spec and revert to direct.
    // The PID loops should keep us from getting to this point, so we should disable if we ever get here.
    // IMU data could also be at fault
    if (rollAngle > 72 || rollAngle < -72 || pitchAngle > 35 || pitchAngle < -20) {
        setIMUSafe(false);
    }

    // Use the rx inputs to set the setpoint control values
    // Deadband calculations so we don't get crazy values due to PWM fluctuations
    if (rollIn > DEADBAND_VALUE || rollIn < -DEADBAND_VALUE) {
        // If the input is not within the deadband, add the smoothed input value on top of the current setpoint
        // We must smooth the value because this calculation is done many times per second, so no smoothing would result
        // in extremely (and I do really mean extreme) touchy controls.
        rollSetpoint += rollIn * SETPOINT_SMOOTHING_VALUE;
    }
    if (pitchIn > DEADBAND_VALUE || pitchIn < -DEADBAND_VALUE) {
        pitchSetpoint += pitchIn * SETPOINT_SMOOTHING_VALUE;
    }
    // Yaw is different--if we detect any aileron input whatsoever, we wil override what PID wants with the user input
    // For this reason, the yaw deadband calculation is towards the end of the function (because we calculate inputs next)

    // Make sure the PID setpoints aren't set to unsafe values so we don't get weird outputs from PID,
    // this is also where our bank/pitch protections come in.
    if (rollSetpoint > ROLL_LIMIT || rollSetpoint < -ROLL_LIMIT) {
        // If the roll values are unsafe, we do allow setting up to 67 but constant input is required, so check for that
        if (!(abs(rollIn) >= abs(rollSetpoint))) {
            if (rollSetpoint > 0) {
                rollSetpoint -= 0.05;
            } else if (rollSetpoint < 0) {
                rollSetpoint += 0.05;
            }
        }
        if (rollSetpoint > ROLL_LIMIT_HOLD) {
            rollSetpoint = ROLL_LIMIT_HOLD;
        } else if (rollSetpoint < -ROLL_LIMIT_HOLD) {
            rollSetpoint = -ROLL_LIMIT_HOLD;
        }
    }
    if (pitchSetpoint > PITCH_UPPER_LIMIT || pitchSetpoint < PITCH_LOWER_LIMIT) {
        // Pitch is simply limited to the unsafe thresholds
        if (pitchSetpoint > PITCH_UPPER_LIMIT) {
            pitchSetpoint = PITCH_UPPER_LIMIT;
        } else if (pitchSetpoint < PITCH_LOWER_LIMIT) {
            pitchSetpoint = PITCH_LOWER_LIMIT;
        }
    }

    // Yaw deadband calculation (described earlier)
    if (yawIn > DEADBAND_VALUE || yawIn < -DEADBAND_VALUE) {
        yawOut = yawIn;
    } else {
        // If there is no user input on rudder axis, first check if we are in a turn or not
        // Keep in mind this is different than if the system is making inputs on ailerons! We only want this to activate during turns, not stabilizations.
        if (rollSetpoint > DEADBAND_VALUE || rollSetpoint < -DEADBAND_VALUE) {
            // If we are in a turn, 
            // first set the yaw damper as disabled,
            yawdamp_on = false;
            // then set the yaw value to a reduced version of our aileron value to coordinate our turn
            yawOut = rollPID.out * RUDDER_TURNING_VALUE;
            // If the yaw setpoint is larger than the limit, bring it back within limits (for the PID controlled outputs this is done within the PID library)
            if (yawOut > RUD_LIMIT) {
                yawOut = RUD_LIMIT;
            } else if (yawOut < -RUD_LIMIT) {
                yawOut = -RUD_LIMIT;
            }
        } else {
            // If the yaw damper is set as off still, that means we have just transitioned to this phase, so we should update the yaw setpoint
            if (!yawdamp_on) {
                yawSetpoint = yawAngle;
            }
            // If not, we must be in level flight, so we will set the PID output values to our output (rudder servo)
            // This is more like an actual yaw damper that helps to eliminate yaw in stable flight
            yawdamp_on = true;
            yawOut = yawPID.out;
        }
    }

    // All input processing is now complete, send the final outputs to the servos
    servo_set(SERVO_AIL_PIN, (uint16_t)(rollPID.out + 90));
    servo_set(SERVO_ELEV_PIN, (uint16_t)(pitchPID.out + 90));
    servo_set(SERVO_RUD_PIN, (uint16_t)(yawOut + 90));
}

void mode_normalReset() {
    rollSetpoint = 0.0f;
    pitchSetpoint = 0.0f;
    yawSetpoint = 0.0f;
    yawdamp_on = false;
}
