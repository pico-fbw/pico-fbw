#pragma once

typedef struct PIDController {

	/* Controller gains */
	double Kp;
	double Ki;
	double Kd;

	/* Derivative low-pass filter time constant */
	double tau;

	/* Output limits */
	double limMin;
	double limMax;
	
	/* Integrator limits */
	double limMinInt;
	double limMaxInt;

	/* Sample time (in seconds) */
	double T;

	/* Controller "memory" */
	double integrator;
	double prevError;			/* Required for integrator */
	double differentiator;
	double prevMeasurement;		/* Required for differentiator */
	double prevT;

	/* Controller output */
	double out;

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
void pid_update(PIDController *pid, double setpoint, double measurement);