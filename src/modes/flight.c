/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdlib.h>

#include "io/aahrs.h"
#include "io/receiver.h"
#include "io/servo.h"

#include "lib/pid.h"

#include "modes/aircraft.h"
#include "modes/tune.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "flight.h"

static PIDController rollC, pitchC, yawC;
static f32 ailOut, eleOut, rudOut;
static f32 lElevonOut, rElevonOut;

static f32 yawOutput;
static f32 flightYawSetpoint;
static bool yawDamperOn;

static void flight_roll_params_update(f64 kP, f64 kI, f64 kD, bool reset) {
    if (kP != INFINITY)
        rollC.Kp = kP;
    if (kI != INFINITY)
        rollC.Ki = kI;
    if (kD != INFINITY)
        rollC.Kd = kD;
    if (reset)
        pid_init(&rollC);
}

static void flight_pitch_params_update(f64 kP, f64 kI, f64 kD, bool reset) {
    if (kP != INFINITY)
        pitchC.Kp = kP;
    if (kI != INFINITY)
        pitchC.Ki = kI;
    if (kD != INFINITY)
        pitchC.Kd = kD;
    if (reset)
        pid_init(&pitchC);
}

void flight_init() {
    f32 rollLimit;
    f32 pitchLimit;
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_3AXIS:
        case CTRLMODE_2AXIS:
            rollLimit = config.control[CONTROL_MAX_AIL_DEFLECTION];
            pitchLimit = config.control[CONTROL_MAX_ELE_DEFLECTION];
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            rollLimit = config.control[CONTROL_MAX_ELEVON_DEFLECTION];
            pitchLimit = config.control[CONTROL_MAX_ELEVON_DEFLECTION];
            break;
        default:
            printpre("flight", "ERROR: unknown control mode!");
            aircraft.change_to(MODE_DIRECT);
            return;
    }
// Create PID controllers for the roll and pitch axes and initialize (also clear) them
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    rollC = (PIDController){calibration.pid[PID_ROLL_KP],
                            calibration.pid[PID_ROLL_KI],
                            calibration.pid[PID_ROLL_KD],
                            calibration.pid[PID_ROLL_TAU],
                            -rollLimit,
                            rollLimit,
                            calibration.pid[PID_ROLL_INTEGMIN],
                            calibration.pid[PID_ROLL_INTEGMAX]};
    pitchC = (PIDController){calibration.pid[PID_PITCH_KP],
                             calibration.pid[PID_PITCH_KI],
                             calibration.pid[PID_PITCH_KD],
                             calibration.pid[PID_PITCH_TAU],
                             -pitchLimit,
                             pitchLimit,
                             calibration.pid[PID_PITCH_INTEGMIN],
                             calibration.pid[PID_PITCH_INTEGMAX]};
    pid_init(&rollC);
    pid_init(&pitchC);
    if (receiver_has_rud()) {
        yawC = (PIDController){calibration.pid[PID_YAW_KP],
                               calibration.pid[PID_YAW_KI],
                               calibration.pid[PID_YAW_KD],
                               calibration.pid[PID_YAW_TAU],
                               -config.control[CONTROL_MAX_RUD_DEFLECTION],
                               config.control[CONTROL_MAX_RUD_DEFLECTION],
                               calibration.pid[PID_YAW_INTEGMIN],
                               calibration.pid[PID_YAW_INTEGMAX]};
        pid_init(&yawC);
    }
#pragma GCC diagnostic pop
}

void flight_update(f64 roll, f64 pitch, f64 yaw, bool override) {
    // Check flight envelope for hard-coded irregularities
    if (fabsf(aahrs.roll) > 72 || aahrs.pitch > 35 || aahrs.pitch < -20) {
        printpre("flight", "WARNING: flight envelope exceeded! (roll: %.0f, pitch: %.0f, yaw: %.0f)", aahrs.roll, aahrs.pitch,
                 aahrs.yaw);
        aircraft.set_aahrs_safe(false);
    }

    // Update PID controllers
    pid_update(&rollC, roll, (f64)aahrs.roll);
    pid_update(&pitchC, pitch, (f64)aahrs.pitch);
    // All control modes require the roll/pitch PIDs to be mapped to a servo output (0-180)
    ailOut = (((bool)config.pins[PINS_REVERSE_ROLL] ? -1 : 1) * (f32)rollC.out + 90.f);
    eleOut = (((bool)config.pins[PINS_REVERSE_PITCH] ? -1 : 1) * (f32)pitchC.out + 90.f);
    // Now things get specific to each mode
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        // Compute yaw damper output for 3axis (rudder-enabled) control modes
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS: {
            if (override) {
                // Yaw override (raw)
                yawOutput = (f32)yaw;
                yawDamperOn = false;
            } else if (fabs(roll) > config.control[CONTROL_DEADBAND]) {
                // Yaw damper disabled (passthrough)
                yawOutput = (f32)(rollC.out * config.control[CONTROL_RUDDER_SENSITIVITY]);
                yawDamperOn = false;
            } else {
                // Yaw damper enabled
                if (!yawDamperOn)
                    flightYawSetpoint = aahrs.yaw; // Yaw damper was just enabled, create our setpoint
                pid_update(&yawC, flightYawSetpoint, aahrs.yaw);
                yawOutput = (f32)yawC.out;
                yawDamperOn = true;
            }

            rudOut = (((bool)config.pins[PINS_REVERSE_YAW] ? -1 : 1) * (f32)yawOutput + 90.f);
            servo_set((u32)config.pins[PINS_SERVO_RUD], rudOut);
        }
        /* fall through */
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            // Send outputs to servos
            servo_set((u32)config.pins[PINS_SERVO_AIL], ailOut);
            servo_set((u32)config.pins[PINS_SERVO_ELE], eleOut);
            break;
        // Flying wing control modes must mix elevator and aileron outputs to create elevon outputs
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING: {
            lElevonOut = control_mix_elevon(LEFT, ailOut, eleOut);
            rElevonOut = control_mix_elevon(RIGHT, ailOut, eleOut);
            // Limit elevon outputs
            clampf(lElevonOut, -config.control[CONTROL_MAX_ELEVON_DEFLECTION], config.control[CONTROL_MAX_ELEVON_DEFLECTION]);
            clampf(rElevonOut, -config.control[CONTROL_MAX_ELEVON_DEFLECTION], config.control[CONTROL_MAX_ELEVON_DEFLECTION]);

            servo_set((u32)config.pins[PINS_SERVO_AIL], lElevonOut);
            servo_set((u32)config.pins[PINS_SERVO_ELE], rElevonOut);
            break;
        }
        default: {
            printpre("flight", "ERROR: unknown control mode!");
            aircraft.change_to(MODE_DIRECT);
            return;
        }
    }
}

void flight_params_get(Axis axis, f64 *kP, f64 *kI, f64 *kD) {
    PIDController *axisC = NULL;
    switch (axis) {
        case ROLL:
            axisC = &rollC;
            break;
        case PITCH:
            axisC = &pitchC;
            break;
    }
    if (axisC == NULL)
        return;
    if (kP != NULL)
        *kP = axisC->Kp;
    if (kI != NULL)
        *kI = axisC->Ki;
    if (kD != NULL)
        *kD = axisC->Kd;
}

void flight_params_update(Axis axis, f64 kP, f64 kI, f64 kD, bool reset) {
    switch (axis) {
        case ROLL:
            flight_roll_params_update(kP, kI, kD, reset);
            break;
        case PITCH:
            flight_pitch_params_update(kP, kI, kD, reset);
            break;
    }
}
