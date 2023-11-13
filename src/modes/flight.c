/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "../lib/pid.h"

#include "../io/aahrs.h"
#include "../io/flash.h"
#include "../io/servo.h"

#include "modes.h"
#include "tune.h"

#include "flight.h"

GPS gps;

static PIDController roll_c;
static PIDController pitch_c;

static PIDController yaw_c;
static float yawOutput;
static float flightYawSetpoint;
static bool yawDamperOn;

static float lElevonOutput;
static float rElevonOutput;

void flight_init() {
    float rollLimit;
    float pitchLimit;
    switch ((ControlMode)flash.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            rollLimit = flash.control[CONTROL_MAX_AIL_DEFLECTION];
            pitchLimit = flash.control[CONTROL_MAX_ELEV_DEFLECTION];
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            rollLimit = flash.control[CONTROL_MAX_ELEVON_DEFLECTION];
            pitchLimit = flash.control[CONTROL_MAX_ELEVON_DEFLECTION];
            break;
    }
    // Create PID controllers for the roll and pitch axes and initialize (also clear) them
    roll_c = (PIDController){flash.pid[PID_ROLL_KP], flash.pid[PID_ROLL_TI], flash.pid[PID_ROLL_TD], flash.pid[PID_ROLL_TAU],
                            -rollLimit, rollLimit, flash.pid[PID_ROLL_INTEGMIN], flash.pid[PID_ROLL_INTEGMAX], flash.pid[PID_ROLL_KT]};
    pitch_c = (PIDController){flash.pid[PID_PITCH_KP], flash.pid[PID_PITCH_TI], flash.pid[PID_PITCH_TD], flash.pid[PID_PITCH_TAU],
                             -pitchLimit, pitchLimit, flash.pid[PID_PITCH_INTEGMIN], flash.pid[PID_PITCH_INTEGMAX], flash.pid[PID_PITCH_KT]};
    pid_init(&roll_c);
    pid_init(&pitch_c);
    if ((ControlMode)flash.general[GENERAL_CONTROL_MODE] == CTRLMODE_3AXIS_ATHR ||
        (ControlMode)flash.general[GENERAL_CONTROL_MODE] == CTRLMODE_3AXIS) {
        yaw_c = (PIDController){flash.pid[PID_YAW_KP], flash.pid[PID_YAW_TI], flash.pid[PID_YAW_TD], flash.pid[PID_YAW_TAU], -flash.control[CONTROL_MAX_RUD_DEFLECTION],
                                flash.control[CONTROL_MAX_RUD_DEFLECTION], flash.pid[PID_YAW_INTEGMIN], flash.pid[PID_YAW_INTEGMAX], flash.pid[PID_PITCH_KT]};
        pid_init(&yaw_c);
    }
}

void flight_update(double roll, double pitch, double yaw, bool override) {
    // Update input data
    // AAHRS is updated asyncronously by core 1; we only need to update GPS when applicable
    if (flash.sensors[SENSORS_GPS_COMMAND_TYPE] != GPS_COMMAND_TYPE_NONE) gps = gps_getData();

    // Check flight envelope for irregularities
    if (fabsf(aahrs.roll) > 72 || aahrs.pitch > 35 || aahrs.pitch < -20) {
        if (print.fbw) printf("WARNING: flight envelope exceeded! (roll: %f, pitch: %f, yaw: %f)\n",
                              aahrs.roll, aahrs.pitch, aahrs.yaw);
        aircraft.setAAHRSSafe(false);
    }

    // Update PID controllers
    pid_update(&roll_c, roll, aahrs.roll);
    pid_update(&pitch_c, pitch, aahrs.pitch);
    switch ((ControlMode)flash.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            // Yaw damper computation
            if (override) {
                // Yaw override (raw)
                yawOutput = (float)yaw;
                yawDamperOn = false;
            } else if (fabs(roll) > flash.control[CONTROL_DEADBAND]) {
                // Yaw damper disabled (passthrough)
                yawOutput = (float)(roll_c.out * flash.control[CONTROL_RUDDER_SENSITIVITY]);
                yawDamperOn = false;
            } else {
                // Yaw damper enabled
                if (!yawDamperOn) {
                    flightYawSetpoint = aahrs.yaw; // Yaw damper was just enabled, create our setpoint
                }
                pid_update(&yaw_c, flightYawSetpoint, aahrs.yaw);
                yawOutput = (float)yaw_c.out;
                yawDamperOn = true;
            }

            // Supply current PID outputs to servos
            servo_set((uint)flash.pins[PINS_SERVO_AIL],
                     (bool)flash.pins[PINS_REVERSE_ROLL] ? (uint16_t)(roll_c.out + 90) : (uint16_t)(roll_c.out - 90));
            servo_set((uint)flash.pins[PINS_SERVO_ELEV],
                     (bool)flash.pins[PINS_REVERSE_PITCH] ? (uint16_t)(pitch_c.out + 90) : (uint16_t)(pitch_c.out - 90));
            servo_set((uint)flash.pins[PINS_SERVO_RUD],
                     (bool)flash.pins[PINS_REVERSE_YAW] ? (uint16_t)(yawOutput + 90) : (uint16_t)(yawOutput - 90));
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            // Control mixing computations
            lElevonOutput = (float)(((bool)flash.pins[PINS_REVERSE_ROLL] ? -1 : 1) * roll_c.out * flash.control[CONTROL_AIL_MIXING_BIAS] +
                            ((bool)flash.pins[PINS_REVERSE_PITCH] ? -1 : 1) * pitch_c.out * flash.control[CONTROL_ELEV_MIXING_BIAS]) *
                            flash.control[CONTROL_ELEVON_MIXING_GAIN];
            rElevonOutput = (float)(((bool)flash.pins[PINS_REVERSE_ROLL] ? -1 : 1) * roll_c.out * flash.control[CONTROL_AIL_MIXING_BIAS] -
                            ((bool)flash.pins[PINS_REVERSE_PITCH] ? -1 : 1) * pitch_c.out * flash.control[CONTROL_ELEV_MIXING_BIAS]) *
                            flash.control[CONTROL_ELEVON_MIXING_GAIN];

            // Limit elevon outputs
            if (fabsf(lElevonOutput) > flash.control[CONTROL_MAX_ELEVON_DEFLECTION]) {
                lElevonOutput = (lElevonOutput > 0) ? flash.control[CONTROL_MAX_ELEVON_DEFLECTION] : -flash.control[CONTROL_MAX_ELEVON_DEFLECTION];
            }
            if (fabsf(rElevonOutput) > flash.control[CONTROL_MAX_ELEVON_DEFLECTION]) {
                rElevonOutput = (rElevonOutput > 0) ? flash.control[CONTROL_MAX_ELEVON_DEFLECTION] : -flash.control[CONTROL_MAX_ELEVON_DEFLECTION];
            }

            servo_set((uint)flash.pins[PINS_SERVO_ELEVON_L], (uint16_t)(lElevonOutput + 90));
            servo_set((uint)flash.pins[PINS_SERVO_ELEVON_R], (uint16_t)(rElevonOutput + 90));
            break;
    }
}
