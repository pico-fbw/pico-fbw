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
#include "../io/pwm.h"
#include "../io/servo.h"

#include "aircraft.h"
#include "tune.h"

#include "flight.h"

static PIDController roll_c, pitch_c, yaw_c;
static uint16_t ailOut, elevOut, rudOut, lElevonOut, rElevonOut;

static float yawOutput;
static float flightYawSetpoint;
static bool yawDamperOn;

void flight_init() {
    float rollLimit;
    float pitchLimit;
    switch ((ControlMode)flash.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_3AXIS:
        case CTRLMODE_2AXIS:
            rollLimit = flash.control[CONTROL_MAX_AIL_DEFLECTION];
            pitchLimit = flash.control[CONTROL_MAX_ELEV_DEFLECTION];
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            rollLimit = flash.control[CONTROL_MAX_ELEVON_DEFLECTION];
            pitchLimit = flash.control[CONTROL_MAX_ELEVON_DEFLECTION];
            break;
        default:
            printf("[flight] ERROR: unknown control mode!\n");
            aircraft.changeTo(MODE_DIRECT);
            return;
    }
    // Create PID controllers for the roll and pitch axes and initialize (also clear) them
    roll_c = (PIDController){flash.pid[PID_ROLL_KP], flash.pid[PID_ROLL_KI], flash.pid[PID_ROLL_KD], flash.pid[PID_ROLL_TAU],
                            -rollLimit, rollLimit, flash.pid[PID_ROLL_INTEGMIN], flash.pid[PID_ROLL_INTEGMAX]};
    pitch_c = (PIDController){flash.pid[PID_PITCH_KP], flash.pid[PID_PITCH_KI], flash.pid[PID_PITCH_KD], flash.pid[PID_PITCH_TAU],
                             -pitchLimit, pitchLimit, flash.pid[PID_PITCH_INTEGMIN], flash.pid[PID_PITCH_INTEGMAX]};
    pid_init(&roll_c);
    pid_init(&pitch_c);
    if (pwm_hasRud()) {
        yaw_c = (PIDController){flash.pid[PID_YAW_KP], flash.pid[PID_YAW_KI], flash.pid[PID_YAW_KD], flash.pid[PID_YAW_TAU], -flash.control[CONTROL_MAX_RUD_DEFLECTION],
                                flash.control[CONTROL_MAX_RUD_DEFLECTION], flash.pid[PID_YAW_INTEGMIN], flash.pid[PID_YAW_INTEGMAX]};
        pid_init(&yaw_c);
    }
}

void flight_update(double roll, double pitch, double yaw, bool override) {
    // Check flight envelope for hard-coded irregularities
    if (fabsf(aahrs.roll) > 72 || aahrs.pitch > 35 || aahrs.pitch < -20) {
        if (print.fbw) printf("[flight] WARNING: flight envelope exceeded! (roll: %f, pitch: %f, yaw: %f)\n",
                              aahrs.roll, aahrs.pitch, aahrs.yaw);
        aircraft.setAAHRSSafe(false);
    }

    // Update PID controllers
    pid_update(&roll_c, roll, (double)aahrs.roll);
    pid_update(&pitch_c, pitch, (double)aahrs.pitch);
    // All control modes require the roll/pitch PIDs to be mapped to a servo output (0-180)
    ailOut = (uint16_t)(((bool)flash.pins[PINS_REVERSE_ROLL] ? -1 : 1) * (int)roll_c.out + 90);
    elevOut = (uint16_t)(((bool)flash.pins[PINS_REVERSE_PITCH] ? -1 : 1) * (int)pitch_c.out + 90);
    // Now things get specific to each mode
    switch ((ControlMode)flash.general[GENERAL_CONTROL_MODE]) {
        // Compute yaw damper output for 3axis (rudder-enabled) control modes
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS: {
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

            rudOut = (uint16_t)(((bool)flash.pins[PINS_REVERSE_YAW] ? -1 : 1) * (int)yawOutput + 90);
            servo_set((uint)flash.pins[PINS_SERVO_RUD], rudOut);
        }
        // Both 3axis and 2axis control modes set to ailerons and elevators, hence no break here
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            // Send outputs to servos
            servo_set((uint)flash.pins[PINS_SERVO_AIL], ailOut);
            servo_set((uint)flash.pins[PINS_SERVO_ELEV], elevOut);
            break;
        // Flying wing control modes must mix elevator and aileron outputs to create elevon outputs
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING: {
            lElevonOut = (((bool)flash.pins[PINS_REVERSE_ROLL] ? -1 : 1) * roll_c.out * flash.control[CONTROL_AIL_MIXING_BIAS] +
                            ((bool)flash.pins[PINS_REVERSE_PITCH] ? -1 : 1) * pitch_c.out * flash.control[CONTROL_ELEV_MIXING_BIAS]) *
                            flash.control[CONTROL_ELEVON_MIXING_GAIN];
            rElevonOut = (((bool)flash.pins[PINS_REVERSE_ROLL] ? -1 : 1) * roll_c.out * flash.control[CONTROL_AIL_MIXING_BIAS] -
                            ((bool)flash.pins[PINS_REVERSE_PITCH] ? -1 : 1) * pitch_c.out * flash.control[CONTROL_ELEV_MIXING_BIAS]) *
                            flash.control[CONTROL_ELEVON_MIXING_GAIN];

            // Limit elevon outputs
            if (fabsf(lElevonOut) > flash.control[CONTROL_MAX_ELEVON_DEFLECTION]) {
                lElevonOut = (lElevonOut > 0) ? flash.control[CONTROL_MAX_ELEVON_DEFLECTION] : -flash.control[CONTROL_MAX_ELEVON_DEFLECTION];
            }
            if (fabsf(rElevonOut) > flash.control[CONTROL_MAX_ELEVON_DEFLECTION]) {
                rElevonOut = (rElevonOut > 0) ? flash.control[CONTROL_MAX_ELEVON_DEFLECTION] : -flash.control[CONTROL_MAX_ELEVON_DEFLECTION];
            }

            servo_set((uint)flash.pins[PINS_SERVO_AIL], lElevonOut + 90);
            servo_set((uint)flash.pins[PINS_SERVO_ELEV], rElevonOut + 90);
            break;
        }
        default: {
            printf("[flight] ERROR: unknown control mode!\n");
            aircraft.changeTo(MODE_DIRECT);
            return;
        }
    }
}
