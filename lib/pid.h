#pragma once

#include "platform/types.h"

typedef struct PIDController {
	f64 kp; // Proportional gain (read-only)
	f64 ki; // Integral gain (read-only)
	f64 kd; // Derivative gain (read-only)

	f64 tau; // Derivative low-pass filter time constant (read-only)

	f64 limMin; // Minimum output value (read-only)
	f64 limMax; // Maximum output value (read-only)

	f64 out; // Controller output (read-only)

	/* private */
	
	f64 T;
	f64 integrator;
	f64 prevError;
	f64 differentiator;
	f64 prevMeasurement;
	f64 prevT;
} PIDController;

/**
 * Initalizes a PIDController.
 * @param pid Pointer to the PIDController to initialize.
 */
void pid_init(PIDController *pid);

/**
 * Updates a PIDController.
 * @param pid Pointer to the PIDController to update.
 * @param setpoint The target value.
 * @param measurement The measured value.
 */
void pid_update(PIDController *pid, f64 setpoint, f64 measurement);
