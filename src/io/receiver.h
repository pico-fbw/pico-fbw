#pragma once

#include <stdbool.h>
#include "platform/int.h"

typedef enum ControlMode {
    CTRLMODE_3AXIS_ATHR,
    CTRLMODE_3AXIS,
    CTRLMODE_2AXIS_ATHR,
    CTRLMODE_2AXIS,
    CTRLMODE_FLYINGWING_ATHR,
    CTRLMODE_FLYINGWING,
} ControlMode;

typedef enum ReceiverMode {
    RECEIVER_MODE_DEGREE,
    RECEIVER_MODE_PERCENT,
} ReceiverMode;

typedef enum ReceiverCalibrationStatus {
    RECEIVERCALIBRATION_OK,
    RECEIVERCALIBRATION_INCOMPLETE,
    RECEIVERCALIBRATION_INVALID,
} ReceiverCalibrationStatus;

/**
 * Enables receiver input functionality on the specified pins (up to 7).
 * @param pins the list of pins to enable PWM input on
 * @param numPins the number of pins you are enabling PWM input on (1-7)
 */
void receiver_enable(u32 pins[], u32 num_pins);

/**
 * @param pin the GPIO pin to read (must have been already enabled with `receiver_enable()`)
 * @param mode the mode of the PWM (DEG or ESC)
 * @return the calculated degree value derived from the pulsewidth on that pin.
 * @note The mode simply changes how data is displayed and not how it is calculated (DEG from 0-180 and ESC from 0-100).
 */
float receiver_get(u32 pin, ReceiverMode mode);

/**
 * Samples a list of pins for deviation from a specified value for a specified number of samples, then saves that offset value
 * to flash.
 * @param pins the list of pins to calibrate
 * @param num_pins the number of pins in the list
 * @param deviations the value we should be seeing on each pin
 * @param num_samples the number of times to sample the pin for deviation
 * @param sample_delay_ms the delay between samples
 * @param run_times the amount of times to run a sampling function (num_samples), will be averaged at the end
 * @return true if the calibration was successful, false if not
 */
bool receiver_calibrate(u32 pins[], u32 num_pins, float deviations[], u32 num_samples, u32 sample_delay_ms, u32 run_times);

/**
 * @return the status of any previous receiver calibration.
 */
ReceiverCalibrationStatus receiver_is_calibrated();

/**
 * Gets the GPIO pins, number of pins, and their deviations designated for the receiver (PWM) in the config.
 * @param pins array of at least 5 elements to fill with pins
 * @param num_pins pointer to the number of pins
 * @param deviations array of at least 5 elements to fill with pin calibration deviations
 */
void receiver_get_pins(u32 *pins, u32 *num_pins, float *deviations);

/**
 * @return true if PWM has been set up with an autothrottle input (aka an autothrottle control mode has been selected), false if
 * not.
 */
bool receiver_has_athr();

/**
 * @return true if PWM has been set up with a rudder input (aka a rudder control mode has been selected), false if not.
 */
bool receiver_has_rud();
