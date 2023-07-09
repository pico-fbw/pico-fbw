/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Markus Hintersteiner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * **/

/**
 * Credit goes to "pms67" and "drbitboy" for developing this PID implementation, check it out here:
 * https://github.com/drbitboy/PID
*/

#ifndef __PID_H
#define __PID_H

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

#endif // __PID_H
