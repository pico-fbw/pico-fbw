#pragma once

#include "platform/types.h"

#define MAX_RECEIVER_PINS 5 // The maximum number of pins that can be used for receiver input at once
#define DEFAULT_RECEIVER_CALIBRATION_SAMPLES 2000
#define DEFAULT_RECEIVER_CALIBRATION_SAMPLE_DELAY_MS 2
#define DEFAULT_RECEIVER_CALIBRATION_RUN_TIMES 3

#define CTRLMODE_MIN CTRLMODE_3AXIS_ATHR
typedef enum ControlMode {
    CTRLMODE_3AXIS_ATHR,
    CTRLMODE_3AXIS,
    CTRLMODE_2AXIS_ATHR,
    CTRLMODE_2AXIS,
    CTRLMODE_FLYINGWING_ATHR,
    CTRLMODE_FLYINGWING,
} ControlMode;
#define CTRLMODE_MAX CTRLMODE_FLYINGWING

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
 * Enables receiver input functionality on the specified pins (up to `MAX_RECEIVER_PINS`).
 * @param pins the list of pins to enable PWM input on
 * @param numPins the number of pins you are enabling PWM input on
 */
void receiver_enable(const i16 pins[], u32 num_pins);

/**
 * @param pin the GPIO pin to read (must have been already enabled with `receiver_enable()`)
 * @param mode the mode of the PWM (DEG or ESC)
 * @return the calculated degree value derived from the pulsewidth on that pin
 * @note The mode simply changes how data is displayed and not how it is calculated (DEG from 0-180 and ESC from 0-100).
 */
f32 receiver_get(i16 pin, ReceiverMode mode);

/**
 * Samples a list of pins for deviation from a specified value for a specified number of samples, then saves that offset
 * value to flash.
 * @param pins the list of pins to calibrate
 * @param num_pins the number of pins in the list
 * @param deviations the value we should be seeing on each pin
 * @param num_samples the number of times to sample the pin for deviation
 * @param sample_delay_ms the delay between samples
 * @param run_times the amount of times to run a sampling function (num_samples), will be averaged at the end
 * @return true if the calibration was successful, false if not
 */
bool receiver_calibrate(const i16 pins[], u32 num_pins, f32 deviations[], u32 num_samples, u32 sample_delay_ms,
                        u32 run_times);

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
void receiver_get_pins(i16 *pins, u32 *num_pins, f32 *deviations);

/**
 * @return true if PWM has been set up with an autothrottle input (aka an autothrottle control mode has been selected),
 * false if not.
 */
bool receiver_has_athr();

/**
 * @return true if PWM has been set up with a rudder input (aka a rudder control mode has been selected), false if not.
 */
bool receiver_has_rud();
