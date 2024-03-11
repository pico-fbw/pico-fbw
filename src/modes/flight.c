/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "platform/int.h"

#include "io/aahrs.h"
#include "io/receiver.h"
#include "io/servo.h"

#include "lib/pid.h"

#include "modes/aircraft.h"
#include "modes/tune.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "flight.h"

static PIDController roll_c, pitch_c, yaw_c;
static float ailOut, elevOut, rudOut;
static float lElevonOut, rElevonOut;

static float yawOutput;
static float flightYawSetpoint;
static bool yawDamperOn;

void flight_init() {
    float rollLimit;
    float pitchLimit;
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_3AXIS:
        case CTRLMODE_2AXIS:
            rollLimit = config.control[CONTROL_MAX_AIL_DEFLECTION];
            pitchLimit = config.control[CONTROL_MAX_ELEV_DEFLECTION];
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            rollLimit = config.control[CONTROL_MAX_ELEVON_DEFLECTION];
            pitchLimit = config.control[CONTROL_MAX_ELEVON_DEFLECTION];
            break;
        default:
            print("[flight] ERROR: unknown control mode!");
            aircraft.changeTo(MODE_DIRECT);
            return;
    }
    // Create PID controllers for the roll and pitch axes and initialize (also clear) them
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    roll_c = (PIDController){calibration.pid[PID_ROLL_KP],
                             calibration.pid[PID_ROLL_KI],
                             calibration.pid[PID_ROLL_KD],
                             calibration.pid[PID_ROLL_TAU],
                             -rollLimit,
                             rollLimit,
                             calibration.pid[PID_ROLL_INTEGMIN],
                             calibration.pid[PID_ROLL_INTEGMAX]};
    pitch_c = (PIDController){calibration.pid[PID_PITCH_KP],
                              calibration.pid[PID_PITCH_KI],
                              calibration.pid[PID_PITCH_KD],
                              calibration.pid[PID_PITCH_TAU],
                              -pitchLimit,
                              pitchLimit,
                              calibration.pid[PID_PITCH_INTEGMIN],
                              calibration.pid[PID_PITCH_INTEGMAX]};
    pid_init(&roll_c);
    pid_init(&pitch_c);
    if (receiver_has_rud()) {
        yaw_c = (PIDController){calibration.pid[PID_YAW_KP],
                                calibration.pid[PID_YAW_KI],
                                calibration.pid[PID_YAW_KD],
                                calibration.pid[PID_YAW_TAU],
                                -config.control[CONTROL_MAX_RUD_DEFLECTION],
                                config.control[CONTROL_MAX_RUD_DEFLECTION],
                                calibration.pid[PID_YAW_INTEGMIN],
                                calibration.pid[PID_YAW_INTEGMAX]};
        pid_init(&yaw_c);
    }
    #pragma GCC diagnostic pop
}

void flight_update(double roll, double pitch, double yaw, bool override) {
    // Check flight envelope for hard-coded irregularities
    if (fabsf(aahrs.roll) > 72 || aahrs.pitch > 35 || aahrs.pitch < -20) {
        print("[flight] WARNING: flight envelope exceeded! (roll: %.0f, pitch: %.0f, yaw: %.0f)", aahrs.roll, aahrs.pitch, aahrs.yaw);
        aircraft.setAAHRSSafe(false);
    }

    // Update PID controllers
    pid_update(&roll_c, roll, (double)aahrs.roll);
    pid_update(&pitch_c, pitch, (double)aahrs.pitch);
    // All control modes require the roll/pitch PIDs to be mapped to a servo output (0-180)
    ailOut = (((bool)config.pins[PINS_REVERSE_ROLL] ? -1 : 1) * (float)roll_c.out + 90.f);
    elevOut = (((bool)config.pins[PINS_REVERSE_PITCH] ? -1 : 1) * (float)pitch_c.out + 90.f);
    // Now things get specific to each mode
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        // Compute yaw damper output for 3axis (rudder-enabled) control modes
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS: {
            if (override) {
                // Yaw override (raw)
                yawOutput = (float)yaw;
                yawDamperOn = false;
            } else if (fabs(roll) > config.control[CONTROL_DEADBAND]) {
                // Yaw damper disabled (passthrough)
                yawOutput = (float)(roll_c.out * config.control[CONTROL_RUDDER_SENSITIVITY]);
                yawDamperOn = false;
            } else {
                // Yaw damper enabled
                if (!yawDamperOn)
                    flightYawSetpoint = aahrs.yaw; // Yaw damper was just enabled, create our setpoint
                pid_update(&yaw_c, flightYawSetpoint, aahrs.yaw);
                yawOutput = (float)yaw_c.out;
                yawDamperOn = true;
            }

            rudOut = (((bool)config.pins[PINS_REVERSE_YAW] ? -1 : 1) * (float)yawOutput + 90.f);
            servo_set((u32)config.pins[PINS_SERVO_RUD], rudOut);
        }
        /* fall through */
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            // Send outputs to servos
            servo_set((u32)config.pins[PINS_SERVO_AIL], ailOut);
            servo_set((u32)config.pins[PINS_SERVO_ELE], elevOut);
            break;
        // Flying wing control modes must mix elevator and aileron outputs to create elevon outputs
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING: {
            lElevonOut =
                (((bool)config.pins[PINS_REVERSE_ROLL] ? -1 : 1) * roll_c.out * config.control[CONTROL_AIL_MIXING_BIAS] +
                 ((bool)config.pins[PINS_REVERSE_PITCH] ? -1 : 1) * pitch_c.out * config.control[CONTROL_ELEV_MIXING_BIAS]) *
                config.control[CONTROL_ELEVON_MIXING_GAIN];
            rElevonOut =
                (((bool)config.pins[PINS_REVERSE_ROLL] ? -1 : 1) * roll_c.out * config.control[CONTROL_AIL_MIXING_BIAS] -
                 ((bool)config.pins[PINS_REVERSE_PITCH] ? -1 : 1) * pitch_c.out * config.control[CONTROL_ELEV_MIXING_BIAS]) *
                config.control[CONTROL_ELEVON_MIXING_GAIN];

            // Limit elevon outputs
            if (fabsf(lElevonOut) > config.control[CONTROL_MAX_ELEVON_DEFLECTION]) {
                lElevonOut = (lElevonOut > 0) ? config.control[CONTROL_MAX_ELEVON_DEFLECTION]
                                              : -config.control[CONTROL_MAX_ELEVON_DEFLECTION];
            }
            if (fabsf(rElevonOut) > config.control[CONTROL_MAX_ELEVON_DEFLECTION]) {
                rElevonOut = (rElevonOut > 0) ? config.control[CONTROL_MAX_ELEVON_DEFLECTION]
                                              : -config.control[CONTROL_MAX_ELEVON_DEFLECTION];
            }

            servo_set((u32)config.pins[PINS_SERVO_AIL], lElevonOut + 90.f);
            servo_set((u32)config.pins[PINS_SERVO_ELE], rElevonOut + 90.f);
            break;
        }
        default: {
            print("[flight] ERROR: unknown control mode!");
            aircraft.changeTo(MODE_DIRECT);
            return;
        }
    }
}
