/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
*/

/**
 * Credit goes to pms67 and drbitboy for developing this PID implementation, check it out here:
 * https://github.com/drbitboy/PID
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdio.h>
#include "pico/time.h"

#include "io/flash.h"

#include "lib/pid.h"

void pid_init(PIDController *pid) {
	if (print.fbw) printf("[pid] initializing a PID controller\n");
	// Clear controller variables
	pid->integrator = 0.0f;
	pid->prevError  = 0.0f;
	pid->differentiator  = 0.0f;
	pid->prevMeasurement = 0.0f;
	pid->out = 0.0f;
}

void pid_update(PIDController *pid, double setpoint, double measurement) {
	// Time
	pid->T = time_us_64() / 1E6 - pid->prevT;

	// Error signal
	double error = setpoint - measurement;

	// Proportional
	double proportional = pid->Kp * error;

	// Integral
	pid->integrator = pid->integrator + 0.5f * pid->Ki * pid->T * (error + pid->prevError);

	// Derivative (band-limited differentiator)
	pid->differentiator = -(2.0f * pid->Kd * (measurement - pid->prevMeasurement) // Derivative on measurement, therefore minus sign in front of equation
    					  + (2.0f * pid->tau - pid->T) * pid->differentiator)
    					  / (2.0f * pid->tau + pid->T);


	// Compute output and apply limits
	pid->out = proportional + pid->integrator + pid->differentiator;
	if (pid->out > pid->limMax) {
		// Anti-wind-up for over-saturated output
		pid->integrator += pid->limMax - pid->out;
		pid->out = pid->limMax;
	} else if (pid->out < pid->limMin) {
		// Anti-wind-up for under-saturated output
		pid->integrator += pid->limMin - pid->out;
		pid->out = pid->limMin;
	}

	// Store error, measurement, and time for later use
	pid->prevError       = error;
	pid->prevMeasurement = measurement;
	pid->prevT = time_us_64() / 1E6;
}
