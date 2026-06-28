/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
 */

/**
 * Credit goes to pms67 and drbitboy for developing this PID implementation, check it out here:
 * https://github.com/drbitboy/PID
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/time.h"

#include "pid.h"

void pid_init(PIDController *pid) {
    // Clear controller variables
    pid->integrator = 0.0;
    pid->prevError = 0.0;
    pid->differentiator = 0.0;
    pid->prevMeasurement = 0.0;
    pid->out = 0.0;
    // Set time
    pid->prevT = time_s();
}

void pid_update(PIDController *pid, f64 setpoint, f64 measurement) {
    pid->T = time_s() - pid->prevT;
    f64 error = setpoint - measurement;

    // Compute PID components
    f64 proportional = pid->kp * error;
    // Integrator with deadband to prevent oscillation around setpoint
    if (pid->deadband == 0.0 || fabs(error) > pid->deadband) {
        pid->integrator = pid->integrator + 0.5 * pid->ki * pid->T * (error + pid->prevError);
    } else {
        // Bleed off integrator slowly when inside deadband to avoid a step when re-entering
        pid->integrator *= 0.98;
    }
    // Derivative (band-limited differentiator)
    // Only recompute derivative when measurement has actually changed
    // This is mainly a problem in the sim where data updates much slower than the PID loop, and therefore
    // != (exact floating point) is a valid check. In real life, sensor noise will almost always cause this check to fail
    if (measurement != pid->prevMeasurement) {
        // Derivative on measurement, therefore minus sign in front of equation
        pid->differentiator =
            -(2.0 * pid->kd * (measurement - pid->prevMeasurement) + (2.0 * pid->tau - pid->T) * pid->differentiator) /
            (2.0 * pid->tau + pid->T);
    }

    // Compute output and apply limits
    f64 out = proportional + pid->integrator + pid->differentiator;
    if (out > pid->limMax) {
        // Anti-wind-up for over-saturated output
        if (pid->integrator != 0.0) {
            pid->integrator += pid->limMax - out;
        }
        out = pid->limMax;
    } else if (out < pid->limMin) {
        // Anti-wind-up for under-saturated output
        if (pid->integrator != 0.0) {
            pid->integrator += pid->limMin - out;
        }
        out = pid->limMin;
    }

    // Store output, error, measurement, and time for later use
    pid->out = out;
    pid->prevError = error;
    pid->prevMeasurement = measurement;
    pid->prevT = time_s();
}
