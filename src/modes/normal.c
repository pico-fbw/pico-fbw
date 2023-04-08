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
PID_TypeDef rollPID;
PID_TypeDef pitchPID;

float rollAngle;
float pitchAngle;
float rollIn;
float pitchIn;

float rollSetpoint;
float pitchSetpoint;
float rollOut;
float pitchOut;

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
    // Deadband calculation so we don't get crazy values due to PWM fluctuations
    if (rollIn > DEADBAND_VALUE || rollIn < -DEADBAND_VALUE) {
        rollSetpoint += rollIn * SETPOINT_SMOOTHING_VALUE;
    }
    if (pitchIn > DEADBAND_VALUE || pitchIn < -DEADBAND_VALUE) {
        pitchSetpoint += pitchIn * SETPOINT_SMOOTHING_VALUE;
    }
    // Make sure the PID setpoints aren't set to unsafe values
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
    servo_set(SERVO_AIL_PIN, (uint16_t)(rollSetpoint + 90));
    servo_set(SERVO_ELEV_PIN, (uint16_t) pitchOut);
    // For now, normal mode does not control the rudder and simply passes it through from the user,
    // mostly because I'm too dumb to understand aerodynamics to implement that (yet!)
    servo_set(SERVO_RUD_PIN, pwm_readDeg(2) + 90);
}

// Internal function that we will later push to the second core to compute the PID math for all controllers
void computePID() {
    while (true) {
        PID_Compute(&rollPID);
        PID_Compute(&pitchPID);
    }
}

void mode_normalInit() {
    // Set up PID controllers for roll and pitch io
    // TODO: make some sample PID tunings (oh no...)
    PID(&rollPID, &rollAngle, &rollOut, &rollSetpoint, 1, 10, 0, _PID_P_ON_E, _PID_CD_DIRECT);
    PID_SetMode(&rollPID, _PID_MODE_AUTOMATIC);
    PID_SetOutputLimits(&rollPID, -70, 70);
    // TODO: not actually entirely sure on what the sample time does, I didn't have to do it last time so try and figure it out for sure
    PID_SetSampleTime(&rollPID, 1);

    PID(&pitchPID, &pitchAngle, &pitchOut, &pitchSetpoint, 1, 10, 0, _PID_P_ON_E, _PID_CD_DIRECT);
    PID_SetMode(&pitchPID, _PID_MODE_AUTOMATIC);
    PID_SetOutputLimits(&pitchPID, -70, 70);
    PID_SetSampleTime(&pitchPID, 1);

    // Wake the second core and tell it to compute PID values, that's all it will be doing
    multicore_launch_core1(computePID);
}