#pragma once

#include <stdbool.h>
#include "platform/int.h"

/**
 * Enables ESC control on a certain pin.
 * @param pin the GPIO pin the ESC is attached to
*/
void esc_enable(u32 pin);

/**
 * Sets the speed of the ESC using the the duty cycle of the PWM signal.
 * @param pin the GPIO pin the ESC is attached to
 * @param speed the speed to set, between 0 and 100
*/
void esc_set(u32 pin, float speed);

/**
 * Calibrates the ESC's throttle detents (IDLE, MCT, MAX) in the config.
 * @param pin the GPIO pin the ESC is attached to
 * @return true if the calibration was successful, false if a timeout occured.
*/
bool esc_calibrate(u32 pin);

/**
 * @return the status of any previous ESC detent calibration.
*/
bool esc_isCalibrated();
