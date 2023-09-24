/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "../lib/pid.h"

#include "../io/flash.h"
#include "../io/servo.h"

#include "modes.h"
#include "tune.h"

#include "../sys/config.h"

#include "flight.h"

Angles aircraft;
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
    switch (config.general.controlMode) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            rollLimit = config.limits.maxAilDeflection;
            pitchLimit = config.limits.maxElevDeflection;
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            rollLimit = config.limits.maxElevonDeflection;
            pitchLimit = config.limits.maxElevonDeflection;
            break;
    }
    // Create PID controllers for the roll and pitch axes and initialize (also clear) them
    roll_c = (PIDController){flash_readFloat(FLOAT_SECTOR_PID, 1), flash_readFloat(FLOAT_SECTOR_PID, 2), flash_readFloat(FLOAT_SECTOR_PID, 3),
    config.pid0.rollTau, -rollLimit, rollLimit, config.pid0.rollIntegMin, config.pid0.rollIntegMax, config.pid0.rollKt};
    pitch_c = (PIDController){flash_readFloat(FLOAT_SECTOR_PID, 4), flash_readFloat(FLOAT_SECTOR_PID, 5), flash_readFloat(FLOAT_SECTOR_PID, 6),
    config.pid0.pitchTau, -pitchLimit, pitchLimit, config.pid0.pitchIntegMin, config.pid0.pitchIntegMax, config.pid0.pitchKt};
    pid_init(&roll_c);
    pid_init(&pitch_c);
    if (config.general.controlMode == CTRLMODE_3AXIS_ATHR || config.general.controlMode == CTRLMODE_3AXIS) {
        yaw_c = (PIDController){config.pid1.yawKp, config.pid1.yawKi, config.pid1.yawKd, config.pid1.yawTau, -config.limits.maxRudDeflection,
        config.limits.maxRudDeflection, config.pid1.yawIntegMin, config.pid1.yawIntegMax, config.pid1.yawKt};
        pid_init(&yaw_c);
    }
}

void flight_update(double roll, double pitch, double yaw, bool override) {
    // Update input data
    aircraft = imu_getAngles();
    if (config.sensors.gpsEnabled) gps = gps_getData();

    // Check flight envelope for irregularities
    if (aircraft.roll > 72 || aircraft.roll < -72 || aircraft.pitch > 35 || aircraft.pitch < -20) {
        if (config.debug.debug_fbw) printf("WARNING: flight envelope exceeded! (roll: %f, pitch: %f, yaw: %f)\n",
        aircraft.roll, aircraft.pitch, aircraft.yaw);
        setIMUSafe(false);
    }

    // Update PID controllers
    pid_update(&roll_c, roll, aircraft.roll);
    pid_update(&pitch_c, pitch, aircraft.pitch);
    switch (config.general.controlMode) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            // Yaw damper computation
            if (override) {
                // Yaw override (raw)
                yawOutput = yaw;
                yawDamperOn = false;
            } else if (roll > config.control.controlDeadband || roll < -config.control.controlDeadband) {
                // Yaw damper disabled (passthrough)
                yawOutput = roll_c.out * config.control.rudderSensitivity;
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
            servo_set(config.pins0.servoAil, config.pins1.reverseRoll ? (uint16_t)(roll_c.out + 90) : (uint16_t)(roll_c.out - 90));
            servo_set(config.pins0.servoElev, config.pins1.reversePitch ? (uint16_t)(pitch_c.out + 90) : (uint16_t)(pitch_c.out - 90));
            servo_set(config.pins0.servoRud, config.pins1.reverseYaw ? (uint16_t)(yawOutput + 90) : (uint16_t)(yawOutput - 90));
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            // Control mixing computations
            lElevonOutput = ((config.pins1.reverseRoll ? -1 : 1) * roll_c.out * config.flyingWing.ailMixingBias + (config.pins1.reversePitch ? -1 : 1) * pitch_c.out * config.flyingWing.elevMixingBias) * config.flyingWing.elevonMixingGain;
            rElevonOutput = ((config.pins1.reverseRoll ? -1 : 1) * roll_c.out * config.flyingWing.ailMixingBias - (config.pins1.reversePitch ? -1 : 1) * pitch_c.out * config.flyingWing.elevMixingBias) * config.flyingWing.elevonMixingGain;

            // Limit elevon outputs
            if (abs(lElevonOutput) > config.limits.maxElevonDeflection) {
                lElevonOutput = (lElevonOutput > 0) ? config.limits.maxElevonDeflection : -config.limits.maxElevonDeflection;
            }
            if (abs(rElevonOutput) > config.limits.maxElevonDeflection) {
                rElevonOutput = (rElevonOutput > 0) ? config.limits.maxElevonDeflection : -config.limits.maxElevonDeflection;
            }

            servo_set(config.pins1.servoElevonL, (uint16_t)(lElevonOutput + 90));
            servo_set(config.pins1.servoElevonR, (uint16_t)(rElevonOutput + 90));
            break;
    }
}
