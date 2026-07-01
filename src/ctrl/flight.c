/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include <stdlib.h>
#include "platform/helpers.h"

#include "ctrl/aircraft.h"
#include "io/imu.h"
#include "io/receiver.h"
#include "io/servo.h"
#include "lib/pid.h"
#include "modes/tune.h"
#include "sys/configuration.h"
#include "sys/print.h"

#include "flight.h"

// Hysteresis thresholds for yaw damper engagement/disengagement
#define YAW_DAMPER_ON_THRESHOLD  (config.control[CONTROL_DEADBAND])
#define YAW_DAMPER_OFF_THRESHOLD (config.control[CONTROL_DEADBAND] * 2.f)
#define YAW_DAMPER_RATE_THRESHOLD 5.f // deg/s, must be below this to consider "not actively rolling"

static PIDController rollC, pitchC, yawC;
static f32 ailOut, eleOut, rudOut;
static f32 lElevonOut, rElevonOut;
static bool yawDamperOn;

// Updates PID tunings of the roll controller
static void roll_tunings_update(f64 kP, f64 kI, f64 kD, bool reset) {
    if (kP != INFINITY) {
        rollC.kp = kP;
    }
    if (kI != INFINITY) {
        rollC.ki = kI;
    }
    if (kD != INFINITY) {
        rollC.kd = kD;
    }
    if (reset) {
        pid_init(&rollC);
    }
}

// Updates PID tunings of the pitch controller
static void pitch_tunings_update(f64 kP, f64 kI, f64 kD, bool reset) {
    if (kP != INFINITY) {
        pitchC.kp = kP;
    }
    if (kI != INFINITY) {
        pitchC.ki = kI;
    }
    if (kD != INFINITY) {
        pitchC.kd = kD;
    }
    if (reset) {
        pid_init(&pitchC);
    }
}

/**
 * Computes yaw output based on current flight conditions.
 * @param roll roll input
 * @param yaw yaw input
 * @param yaw_override whether yaw override is active
 * @return computed yaw output
 */
static f32 compute_yaw_output(f64 roll, f64 yaw, bool yaw_override) {
    // If yaw_override is active, just return the raw value directly
    if (yaw_override) {
        yawDamperOn = false;
        return (f32)yaw;
    }

    // Turn damper off if roll exceeds upper threshold
    if (fabs(roll) > YAW_DAMPER_OFF_THRESHOLD) {
        yawDamperOn = false;
    }
    // Turn damper on only if roll is below lower threshold
    if (!yawDamperOn && fabs(roll) < YAW_DAMPER_ON_THRESHOLD && fabs(imu.rollRate) < YAW_DAMPER_RATE_THRESHOLD) {
        pid_init(&yawC); // Reset integrator on re-engage to avoid step
        yawDamperOn = true;
    }
    if (yawDamperOn) {
        pid_update(&yawC, 0.0, (f64)imu.yawRate);
        return (f32)yawC.out;
    }

    // Yaw damper disabled, pass through active roll rate coupled with our sensitivity gain to create a coordinate turn
    return (f32)(imu.rollRate * config.control[CONTROL_RUDDER_SENSITIVITY]);
}

// Applies PID output to servo position, accounting for reversal
static inline f32 apply_pid_output(f64 output, bool reversed) {
    return (reversed ? -1.f : 1.f) * (f32)output + 90.f;
}

// Handles control for 2-axis modes
static void handle_mode_2axis() {
    servo_set((i16)config.pins[PINS_SERVO_AIL], ailOut);
    servo_set((i16)config.pins[PINS_SERVO_ELE], eleOut);
}

// Handles control for 3-axis modes
static void handle_mode_3axis(f64 roll, f64 yaw, bool yaw_override) {
    rudOut = apply_pid_output(compute_yaw_output(roll, yaw, yaw_override), (bool)config.pins[PINS_REVERSE_YAW]);
    // printpre("flight", "rudOut: %.3f, roll: %.3f, yaw: %.3f, yawRate: %.3f, damperOn: %s", rudOut, roll, yaw, imu.yawRate, yawDamperOn ? "true" : "false");
    servo_set((i16)config.pins[PINS_SERVO_RUD], rudOut);
    handle_mode_2axis();
}

// Handles control for flying wing modes
static void handle_mode_flyingwing() {
    // Flying wing control modes must mix elevator and aileron outputs to create elevon outputs
    lElevonOut = control_mix_elevon(ELEVON_LEFT, ailOut, eleOut);
    rElevonOut = control_mix_elevon(ELEVON_RIGHT, ailOut, eleOut);
    // Limit elevon outputs
    f32 minOut = 90.f - config.control[CONTROL_MAX_ELEVON_DEFLECTION]; // Bring into servo range
    f32 maxOut = 90.f + config.control[CONTROL_MAX_ELEVON_DEFLECTION];
    clampf(lElevonOut, minOut, maxOut);
    clampf(rElevonOut, minOut, maxOut);

    servo_set((i16)config.pins[PINS_SERVO_AIL], lElevonOut);
    servo_set((i16)config.pins[PINS_SERVO_ELE], rElevonOut);
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
            aircraft_change_mode(MODE_DIRECT);
            return;
    }
    // Create PID controllers for the roll and pitch axes and initialize them
    rollC = (PIDController){
        .kp = calibration.pid[PID_ROLL_KP],
        .ki = calibration.pid[PID_ROLL_KI],
        .kd = calibration.pid[PID_ROLL_KD],
        .deadband = calibration.pid[PID_ROLL_DB],
        .tau = calibration.pid[PID_TAU],
        .limMin = -rollLimit,
        .limMax = rollLimit,
    };
    pitchC = (PIDController){
        .kp = calibration.pid[PID_PITCH_KP],
        .ki = calibration.pid[PID_PITCH_KI],
        .kd = calibration.pid[PID_PITCH_KD],
        .deadband = calibration.pid[PID_PITCH_DB],
        .tau = calibration.pid[PID_TAU],
        .limMin = -pitchLimit,
        .limMax = pitchLimit,
    };
    pid_init(&rollC);
    pid_init(&pitchC);
    // Create yaw axis controller if applicable
    if (receiver_has_rud()) {
        yawC = (PIDController){
            .kp = calibration.pid[PID_YAW_KP],
            .ki = calibration.pid[PID_YAW_KI],
            .kd = calibration.pid[PID_YAW_KD],
            .deadband = calibration.pid[PID_YAW_DB],
            .tau = calibration.pid[PID_TAU],
            .limMin = -config.control[CONTROL_MAX_RUD_DEFLECTION],
            .limMax = config.control[CONTROL_MAX_RUD_DEFLECTION],
        };
        pid_init(&yawC);
    }
}

void flight_update(f64 roll, f64 pitch, f64 yaw, bool yaw_override) {
    // Check flight envelope for hard-coded irregularities
    if (fabsf(imu.roll) > 72 || imu.pitch > 35 || imu.pitch < -30) {
        printpre("flight", "WARNING: flight envelope exceeded! (roll: %.0f, pitch: %.0f, yaw: %.0f)", imu.roll,
                 imu.pitch, imu.yaw);
        aircraft_set_imu_safe(false);
    }

    // Update PID controllers
    pid_update(&rollC, roll, (f64)imu.roll);
    pid_update(&pitchC, pitch, (f64)imu.pitch);
    // All control modes require the roll/pitch PIDs to be mapped to a servo output (0-180)
    ailOut = (((bool)config.pins[PINS_REVERSE_ROLL] ? -1 : 1) * (f32)rollC.out + 90.f);
    eleOut = (((bool)config.pins[PINS_REVERSE_PITCH] ? -1 : 1) * (f32)pitchC.out + 90.f);
    // Now things get specific to each mode
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        // Compute yaw damper output for 3axis (rudder-enabled) control modes
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            handle_mode_3axis(roll, yaw, yaw_override);
            break;
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            handle_mode_2axis();
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            handle_mode_flyingwing();
            break;
        default: {
            printpre("flight", "ERROR: unknown control mode!");
            aircraft_change_mode(MODE_DIRECT);
            return;
        }
    }
}

void flight_tunings_get(Axis axis, f64 *kP, f64 *kI, f64 *kD) {
    PIDController *axisC = NULL;
    switch (axis) {
        case AXIS_ROLL:
            axisC = &rollC;
            break;
        case AXIS_PITCH:
            axisC = &pitchC;
            break;
    }
    if (!axisC) {
        return;
    }
    if (kP) {
        *kP = axisC->kp;
    }
    if (kI) {
        *kI = axisC->ki;
    }
    if (kD) {
        *kD = axisC->kd;
    }
}

void flight_tunings_update(Axis axis, f64 kP, f64 kI, f64 kD, bool reset) {
    switch (axis) {
        case AXIS_ROLL:
            roll_tunings_update(kP, kI, kD, reset);
            break;
        case AXIS_PITCH:
            pitch_tunings_update(kP, kI, kD, reset);
            break;
    }
}
