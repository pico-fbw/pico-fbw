/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include "../lib/pid.h"

#include "pico/multicore.h"
#include "hardware/sync.h"

#include "../io/flash.h"
#include "../io/servo.h"

#include "modes.h"
#include "tune.h"

#include "flight.h"

inertialAngles aircraft;
#ifdef GPS_ENABLED
    GPS gps;
#endif

// TODO: look into using gps heading when possible if relying on MPU due to the unreliability of no compass + fusion delays

static PIDController roll_c;
static PIDController pitch_c;
static PIDController yaw_c;
static float yawOutput;
static float flightYawSetpoint;
static bool yawDamperOn;

void flight_init() {
    // Create PID controllers for the roll and pitch axes and initialize (also clears) them
    #ifdef PID_AUTOTUNE
        roll_c = (PIDController){flash_read(FLASH_SECTOR_PID0, 1), flash_read(FLASH_SECTOR_PID0, 2), flash_read(FLASH_SECTOR_PID0, 3), roll_tau, -AIL_LIMIT, AIL_LIMIT, roll_integMin, roll_integMax, roll_kT};
        pitch_c = (PIDController){flash_read(FLASH_SECTOR_PID1, 1), flash_read(FLASH_SECTOR_PID1, 2), flash_read(FLASH_SECTOR_PID1, 3), pitch_tau, -ELEV_LIMIT, ELEV_LIMIT, pitch_integMin, pitch_integMax, pitch_kT};
    #else
        roll_c = (PIDController){roll_kP, roll_kI, roll_kD, roll_tau, -AIL_LIMIT, AIL_LIMIT, roll_integMin, roll_integMax, roll_kT};
        pitch_c = (PIDController){pitch_kP, pitch_kI, pitch_kD, pitch_tau, -ELEV_LIMIT, ELEV_LIMIT, pitch_integMin, pitch_integMax, pitch_kT};
    #endif
    yaw_c = (PIDController){yaw_kP, yaw_kI, yaw_kD, yaw_tau, -RUD_LIMIT, RUD_LIMIT, yaw_integMin, yaw_integMax, yaw_kT};
    pid_init(&roll_c);
    pid_init(&pitch_c);
    pid_init(&yaw_c);
}

void flight_update(double roll, double pitch, double yaw, bool override) {
    // Update input data
    aircraft = imu_getAngles();
    #ifdef GPS_ENABLED
        gps = gps_getData();
    #endif

    // Check flight envelope for irregularities
    if (aircraft.roll > 72 || aircraft.roll < -72 || aircraft.pitch > 35 || aircraft.pitch < -20) {
        setIMUSafe(false);
    }

    // Update PID controllers
    pid_update(&roll_c, roll, aircraft.roll);
    pid_update(&pitch_c, pitch, aircraft.pitch);
    // Yaw damper computations
    if (override) {
        // Yaw override (raw)
        yawOutput = yaw;
        yawDamperOn = false;
    } else if (roll > DEADBAND_VALUE || roll < -DEADBAND_VALUE) {
        // Yaw damper disabled (passthrough)
        yawOutput = roll_c.out * RUDDER_TURNING_VALUE;
        yawDamperOn = false;
    } else {
        // Yaw damper enabled
        if (!yawDamperOn) {
            flightYawSetpoint = aircraft.yaw; // Yaw damper was just enabled, create our setpoint
        }
        pid_update(&yaw_c, flightYawSetpoint, aircraft.yaw);
        yawOutput = yaw_c.out;
        yawDamperOn = true;
    }

    // Supply current PID outputs to servos
    servo_set(SERVO_AIL_PIN, (uint16_t)(roll_c.out + 90));
    servo_set(SERVO_ELEV_PIN, (uint16_t)(pitch_c.out + 90));
    servo_set(SERVO_RUD_PIN, (uint16_t)(yawOutput + 90));
}

// TODO: multicore disabled for now, it was causing too many issues lol
