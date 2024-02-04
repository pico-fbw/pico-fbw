#ifndef __PWM_H
#define __PWM_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/types.h"

typedef enum ControlMode {
    CTRLMODE_3AXIS_ATHR,
    CTRLMODE_3AXIS,
    CTRLMODE_2AXIS_ATHR,
    CTRLMODE_2AXIS,
    CTRLMODE_FLYINGWING_ATHR,
    CTRLMODE_FLYINGWING
} ControlMode;

typedef enum PWMMode {
    PWM_MODE_DEG,
    PWM_MODE_ESC
} PWMMode;

typedef enum PWMCalibrationStatus {
    PWMCALIBRATION_OK,
    PWMCALIBRATION_INCOMPLETE,
    PWMCALIBRATION_INVALID
} PWMCalibrationStatus;

/**
 * @return true if the value is within the maximum calibration offset.
*/
#define WITHIN_MAX_CALIBRATION_OFFSET(value, offset) ((value) >= -offset && (value) <= offset)

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
 * @param pin the GPIO pin to read (must have been already initalized)
 * @param mode the mode of the PWM (DEG or ESC)
 * @return the calculated degree value derived from the pulsewidth on that pin.
 * @note The mode simply changes how data is displayed and not how it is calculated (DEG from 0-180 and ESC from 0-100).
*/
float pwm_read(uint pin, PWMMode mode);

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
bool pwm_calibrate(const uint pin_list[], uint num_pins, const float deviations[], uint num_samples, uint sample_delay_ms, uint run_times);

/**
 * @return the status of any previous PWM calibration.
*/
PWMCalibrationStatus pwm_isCalibrated();

/**
 * Gets the GPIO pins, number of pins, and their deviations designated for PWM in the config.
 * @param pins array of at least 5 elements to fill with pins
 * @param num_pins pointer to the number of pins
 * @param deviations array of at least 5 elements to fill with pin calibration deviations
*/
void pwm_getPins(uint *pins, uint *num_pins, float *deviations);

/**
 * @return true if PWM has been set up with an autothrottle input (aka an autothrottle control mode has been selected), false if not.
*/
bool pwm_hasAthr();

/**
 * @return true if PWM has been set up with a rudder input (aka a rudder control mode has been selected), false if not.
*/
bool pwm_hasRud();

#endif // __PWM_H
