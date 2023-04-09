#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "../io/imu.h"
#include "../io/pwm.h"
#include "../io/servo.h"
#include "../lib/pid.h"
#include "../config.h"
#include "modes.h"

#include "normal.h"

inertialAngles angles;
PIDController rollPID;
PIDController pitchPID;

float rollAngle;
float pitchAngle;
float rollIn;
float pitchIn;

float rollSetpoint;
float pitchSetpoint;

void mode_normal() {
    // Refresh input data from IMU and rx
    angles = imu_getInertialAngles();
    rollAngle = angles.roll;
    pitchAngle = angles.pitch;
    rollIn = pwm_readDeg(0) - 90;
    pitchIn = pwm_readDeg(1) - 90;

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
    // Make sure the PID setpoints aren't set to unsafe values so we don't get weird outputs from PID,
    // this is also where our bank/pitch protections come in.
    if (rollSetpoint > ROLL_UPPER_LIMIT || rollSetpoint < ROLL_LOWER_LIMIT) {
        // If the roll values are unsafe, we do allow setting up to 67 but constant input is required, so check for that
        if (!(abs(rollIn) >= abs(rollSetpoint))) {
            if (rollSetpoint > 0) {
                rollSetpoint -= 0.05;
            } else if (rollSetpoint < 0) {
                rollSetpoint += 0.05;
            }
        }
        if (rollSetpoint > ROLL_UPPER_LIMIT_HOLD) {
            rollSetpoint = ROLL_UPPER_LIMIT_HOLD;
        } else if (rollSetpoint < ROLL_LOWER_LIMIT_HOLD) {
            rollSetpoint = ROLL_LOWER_LIMIT_HOLD;
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

    // All input processing is complete, send the final outputs to the servos
    servo_set(SERVO_AIL_PIN, (uint16_t)(rollPID.out + 90));
    servo_set(SERVO_ELEV_PIN, (uint16_t)(pitchPID.out + 90));
}

// Internal function that we will later push to the second core to compute the PID math for all controllers
void computePID() {
    while (true) {
        pid_update(&rollPID, rollSetpoint, rollAngle);
        pid_update(&pitchPID, pitchSetpoint, pitchAngle);
    }
}

void mode_normalInit() {
    // Set up PID controllers for roll and pitch io
    rollPID = (PIDController){roll_kP, roll_kI, roll_kD, roll_tau, ROLL_LOWER_LIMIT_HOLD, ROLL_UPPER_LIMIT_HOLD, roll_integMin, roll_integMax, roll_kT};
    pid_init(&rollPID);
    pitchPID = (PIDController){pitch_kP, pitch_kI, pitch_kD, pitch_tau, PITCH_LOWER_LIMIT, PITCH_UPPER_LIMIT, pitch_integMin, pitch_integMax, pitch_kT};
    pid_init(&pitchPID);
    // Wake the second core and tell it to compute PID values, that's all it will be doing
    multicore_launch_core1(computePID);
}

void mode_normalReset() {
    rollSetpoint = 0.0f;
    pitchSetpoint = 0.0f;
}