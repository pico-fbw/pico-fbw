#ifndef __PWM_H
#define __PWM_H

typedef enum ControlMode {
    CTRLMODE_3AXIS_ATHR,
    CTRLMODE_3AXIS,
    CTRLMODE_FLYINGWING_ATHR,
    CTRLMODE_FLYINGWING
} ControlMode;

typedef enum PWMMode {
    PWM_MODE_DEG,
    PWM_MODE_ESC
} PWMMode;

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
void pwm_enable(const uint pin_list[], const uint num_pins);

/**
 * @param pin the GPIO pin to read (must have been already initalized)
 * @param mode the mode of the PWM (DEG or ESC)
 * @return the calculated degree value derived from the pulsewidth on that pin.
 * @note The mode simply changes how data is displayed and not how it is calculated (DEG from 0-180 and ESC from 0-100).
*/
float pwm_read(const uint pin, PWMMode mode);

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
bool pwm_calibrate(const uint pin_list[], const uint num_pins, const float deviations[], uint num_samples, uint sample_delay_ms, uint run_times);

/**
 * Checks if the PWM calibration has been run before.
 * @return 0 if calibration has been run previously, -1 if no calibration has been run, and -2 if the calibration was run in a different control mode.
*/
int pwm_isCalibrated();

#endif // __PWM_H
