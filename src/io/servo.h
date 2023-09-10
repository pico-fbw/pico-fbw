#ifndef __SERVO_H
#define __SERVO_H

/**
 * Set the frequency, making "top" as large as possible for maximum resolution.
 * Maximum "top" is set at 65534 to be able to achieve 100% duty with 65535.
*/
#define PWM_TOP_MAX 65534

/**
 * Sets the position of the servo using the the duty cycle of the PWM signal.
 * @param gpio_pin the GPIO pin the servo is attached to
 * @param degree the position in degrees, a value within 0-180
*/
void servo_set(const uint gpio_pin, const uint16_t degree);

/**
 * Enables servo control on a certain pin.
 * @param gpio_pin the GPIO pin the servo is attached to
 * @return 0 if the operation was successful, 1 if the frequency was too small, and 2 if the frequency was too large.
*/
uint servo_enable(const uint gpio_pin);

/**
 * Disables servo control on a certain pin.
 * @param gpio_pin the GPIO pin the servo is attached to
*/
void servo_disable(const uint gpio_pin);

#endif // __SERVO_H
