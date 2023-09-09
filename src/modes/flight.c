/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdlib.h>
#include "../lib/pid.h"

#include "../io/flash.h"
#include "../io/servo.h"

#include "modes.h"
#include "tune.h"

#include "flight.h"

inertialAngles aircraft;
#ifdef GPS_ENABLED
    GPS gps;
#endif

static PIDController roll_c;
static PIDController pitch_c;
#if defined(CONTROL_3AXIS)
    static PIDController yaw_c;
    static float yawOutput;
    static float flightYawSetpoint;
    static bool yawDamperOn;
#elif defined(CONTROL_FLYINGWING)
    static float lElevonOutput;
    static float rElevonOutput;
#endif

void flight_init() {
    // Create PID controllers for the roll and pitch axes and initialize (also clears) them
    #ifdef PID_AUTOTUNE
        roll_c = (PIDController){flash_read(FLASH_SECTOR_PID, 1), flash_read(FLASH_SECTOR_PID, 2), flash_read(FLASH_SECTOR_PID, 3), roll_tau, -MAX_AIL_DEFLECTION, MAX_AIL_DEFLECTION, roll_integMin, roll_integMax, roll_kT};
        pitch_c = (PIDController){flash_read(FLASH_SECTOR_PID, 4), flash_read(FLASH_SECTOR_PID, 5), flash_read(FLASH_SECTOR_PID, 6), pitch_tau, -MAX_ELEV_DEFLECTION, MAX_ELEV_DEFLECTION, pitch_integMin, pitch_integMax, pitch_kT};
    #else
        roll_c = (PIDController){roll_kP, roll_kI, roll_kD, roll_tau, -MAX_AIL_DEFLECTION, MAX_AIL_DEFLECTION, roll_integMin, roll_integMax, roll_kT};
        pitch_c = (PIDController){pitch_kP, pitch_kI, pitch_kD, pitch_tau, -MAX_ELEV_DEFLECTION, MAX_ELEV_DEFLECTION, pitch_integMin, pitch_integMax, pitch_kT};
    #endif
    pid_init(&roll_c);
    pid_init(&pitch_c);
    #ifdef CONTROL_3AXIS
        // Yaw computations are only relavent to aircraft with, well, yaw control aka 3 axis aircraft
        yaw_c = (PIDController){yaw_kP, yaw_kI, yaw_kD, yaw_tau, -MAX_RUD_DEFLECTION, MAX_RUD_DEFLECTION, yaw_integMin, yaw_integMax, yaw_kT};
        pid_init(&yaw_c);
    #endif
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
    #if defined(CONTROL_3AXIS)
        // Yaw damper computation
        if (override) {
            // Yaw override (raw)
            yawOutput = yaw;
            yawDamperOn = false;
        } else if (roll > DEADBAND_VALUE || roll < -DEADBAND_VALUE) {
            // Yaw damper disabled (passthrough)
            yawOutput = roll_c.out * RUD_TURN_SENSITIVITY;
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
    #elif defined(CONTROL_FLYINGWING)
        // Control mixing computations
        lElevonOutput = ((roll_c.out * AIL_MIXING_BIAS) + (pitch_c.out * ELEV_MIXING_BIAS)) * ELEVON_MIXING_GAIN;
        rElevonOutput = ((roll_c.out * AIL_MIXING_BIAS) - (pitch_c.out * ELEV_MIXING_BIAS)) * ELEVON_MIXING_GAIN;
        // Limit elevon outputs
        if (abs(lElevonOutput) > MAX_ELEVON_DEFLECTION) {
            if (lElevonOutput > 0) {
                lElevonOutput = MAX_ELEVON_DEFLECTION;
            } else {
                lElevonOutput = -MAX_ELEVON_DEFLECTION;
            }
        }
        if (abs(rElevonOutput) > MAX_ELEVON_DEFLECTION) {
            if (rElevonOutput > 0) {
                rElevonOutput = MAX_ELEVON_DEFLECTION;
            } else {
                rElevonOutput = -MAX_ELEVON_DEFLECTION;
            }
        }
    #endif

    // Supply current PID outputs to servos
    #if defined(CONTROL_3AXIS)
        servo_set(SERVO_AIL_PIN, (uint16_t)(roll_c.out + 90));
        servo_set(SERVO_ELEV_PIN, (uint16_t)(pitch_c.out + 90));
        servo_set(SERVO_RUD_PIN, (uint16_t)(yawOutput + 90));
    #elif defined(CONTROL_FLYINGWING)
        servo_set(SERVO_ELEVON_L_PIN, (uint16_t)(lElevonOutput + 90));
        servo_set(SERVO_ELEVON_R_PIN, (uint16_t)(rElevonOutput + 90));
    #endif
}
