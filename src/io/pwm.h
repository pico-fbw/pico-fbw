#ifndef __PWM_H
#define __PWM_H

/**
 * @return true if the value is within the maximum calibration offset.
*/
#define WITHIN_MAX_CALIBRATION_OFFSET(value) ((value) >= -MAX_CALIBRATION_OFFSET && (value) <= MAX_CALIBRATION_OFFSET)

/* You may wonder why there is a limit of seven pins even though there are eight state machines.
This is because the Pico W reserves one state machine for itself, and even though we could use the full eight on a regular Pico,
this keeps compatability between models. */

/**
 * Enables PWM input functionality on the specified pins.
 * Up to seven pins can be enabled; attempting to use over four will use PIO1 in addition to PIO0 by default.
 * @param pin_list the list of pins to enable PWM input on
 * @param num_pins the number of pins you are enabling PWM input on (1-7)
*/
void pwm_enable(uint pin_list[], uint num_pins);

/**
 * @param pin the the GPIO pin to read (must have been already initalized)
 * @return the calculated degree value derived from the pulsewidth on that pin.
*/
float pwm_readDeg(uint pin);

/**
 * Samples a list of pins for deviation from a specified value for a specified number of samples, then saves that offset value to flash.
 * @param pin_list the list of pins to calibrate
 * @param num_pins the number of pins in the list
 * @param deviations the value we should be seeing on each pin
 * @param num_samples the number of times to sample the pin for deviation
 * @param sample_delay_ms the delay between samples
 * @param run_times the amount of times to run a sampling function (num_samples), will be averaged at the end
 * 
 * @return true if the calibration was successful, false if not
*/
bool pwm_calibrate(uint pin_list[], uint num_pins, float deviations[], uint num_samples, uint sample_delay_ms, uint run_times);

/**
 * Checks if the PWM calibration has been run before.
 * @return 0 if calibration has been run previously, -1 if no calibration has been run, and -2 if calibration values seem abnormal.
*/
int pwm_isCalibrated();

#endif // __PWM_H
