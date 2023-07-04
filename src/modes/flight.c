/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>

#include "../io/flash.h"
#include "../io/servo.h"
#include "../lib/pid.h"

#include "../config.h"

#include "modes.h"
#include "tune.h"

#include "flight.h"

inertialAngles aircraft;

static PIDController roll;
static PIDController pitch;
static PIDController yaw;

static float yawOutput;
static float flightYawSetpoint;
static bool yawDamperOn;

void flight_init() {
    // Create PID controllers for the roll and pitch axes and initialize (clear) them
    #ifdef PID_AUTOTUNE
        roll = (PIDController){flash_read(1, 1), flash_read(1, 2), flash_read(1, 3), roll_tau, -AIL_LIMIT, AIL_LIMIT, roll_integMin, roll_integMax, roll_kT};
        pitch = (PIDController){flash_read(2, 1), flash_read(2, 2), flash_read(2, 3), pitch_tau, -ELEV_LIMIT, ELEV_LIMIT, pitch_integMin, pitch_integMax, pitch_kT};
    #else
        roll = (PIDController){roll_kP, roll_kI, roll_kD, roll_tau, -AIL_LIMIT, AIL_LIMIT, roll_integMin, roll_integMax, roll_kT};
        pitch = (PIDController){pitch_kP, pitch_kI, pitch_kD, pitch_tau, -ELEV_LIMIT, ELEV_LIMIT, pitch_integMin, pitch_integMax, pitch_kT};
    #endif
    yaw = (PIDController){yaw_kP, yaw_kI, yaw_kD, yaw_tau, -RUD_LIMIT, RUD_LIMIT, yaw_integMin, yaw_integMax, yaw_kT};
    pid_init(&roll);
    pid_init(&pitch);
    pid_init(&yaw);
}

void flight_update_core0() {
    // Update aircraft data and check flight envelope
    aircraft = imu_getAngles();
    if (aircraft.roll > 72 || aircraft.roll < -72 || aircraft.pitch > 35 || aircraft.pitch < -20) {
        setIMUSafe(false);
    }
    // Supply current PID outputs to servos
    servo_set(SERVO_AIL_PIN, (uint16_t)(roll.out + 90));
    servo_set(SERVO_ELEV_PIN, (uint16_t)(roll.out + 90));
    servo_set(SERVO_RUD_PIN, (uint16_t)(yawOutput + 90));
}

void flight_update_core1(double rollSetpoint, double pitchSetpoint, double yawSetpoint, bool yawOverride) {
    // Update PIDs with new supplied data
    pid_update(&roll, rollSetpoint, aircraft.roll);
    pid_update(&pitch, pitchSetpoint, aircraft.pitch);
    if (yawOverride) {
        // Yaw override (raw)
        yawOutput = yawSetpoint;
        yawDamperOn = false;
    } else if (rollSetpoint > DEADBAND_VALUE || rollSetpoint < -DEADBAND_VALUE) {
        // Yaw damper disabled (passthrough)
        yawOutput = roll.out * RUDDER_TURNING_VALUE;
        yawDamperOn = false;
    } else {
        // Yaw damper enabled
        if (!yawDamperOn) {
            flightYawSetpoint = aircraft.yaw; // Yaw damper was just enabled, create our setpoint
        }
        pid_update(&yaw, flightYawSetpoint, aircraft.yaw);
        yawOutput = yaw.out;
        yawDamperOn = true;
    }
}
