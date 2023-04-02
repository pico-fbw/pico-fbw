#include "pico/stdlib.h"

#include "../io/imu.h"
#include "../io/servo.h"
#include "../config.h"

#include "normal.h"

inertialAngles angles;

void mode_normal() {
    angles = imu_getInertialAngles();
    servo_set(SERVO_AIL_PIN, angles.roll + 90);
    servo_set(SERVO_ELEV_PIN, angles.pitch + 90);
    servo_set(SERVO_RUD_PIN, angles.heading + 90);

    // TODO: PID control with gyro w/ integrated user input (obviously), and checking for if values are wayyy too out of spec and revert to direct
    // (like > 70 bank, -15 < 30 pitch, etc. b/c thats what airbus has), because the system's thresholds should keep it from going that far
    // so if it's gone that far it should step down
}