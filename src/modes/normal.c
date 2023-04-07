#include "pico/stdlib.h"

#include "../io/imu.h"
#include "../io/pwm.h"
#include "../io/servo.h"
#include "../config.h"

#include "normal.h"

inertialAngles angles;
float rollAngle;
float pitchAngle;
float yawAngle;
float rollIn;
float pitchIn;
float yawIn;

uint16_t rollOut;
uint16_t pitchOut;
uint16_t yawOut;

void mode_normal() {
    // Get input data from IMU and rx
    angles = imu_getInertialAngles();
    rollAngle = angles.roll;
    pitchAngle = angles.pitch;
    yawAngle = angles.heading;
    rollIn = pwm_readDeg(0);
    pitchIn = pwm_readDeg(1);
    yawIn = pwm_readDeg(2);

    // TODO: PID control with gyro w/ integrated user input (obviously), and checking for if values are wayyy too out of spec and revert to direct
    // (like > 70 bank, -15 < 30 pitch, etc. b/c thats what airbus has), because the system's thresholds should keep it from going that far
    // so if it's gone that far it should step down

    servo_set(SERVO_AIL_PIN, rollOut;
    servo_set(SERVO_ELEV_PIN, angles.pitch + 90);
    servo_set(SERVO_RUD_PIN, angles.heading + 90);
}