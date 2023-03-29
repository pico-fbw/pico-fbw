/**
 * Huge thanks to 'markushi' on GitHub for developing the bulk of this servo library! (slightly modified by MylesAndMore)
 * Check them out here: https://github.com/markushi/pico-servo
*/

#ifndef servo_h
#define servo_h

/**
 * Set the frequency, making "top" as large as possible for maximum resolution.
 * Maximum "top" is set at 65534 to be able to achieve 100% duty with 65535.
*/
#define SERVO_TOP_MAX 65534

/**
 * Enable Servo control.
 * @param gpio_pin: the GPIO pin the servo is attached to
 * 
 * @return 0 if the operation was successful
*/
int servo_enable(const uint gpio_pin);

/**
 * Enable Servo control.
 * @param gpio_pin: the GPIO pin the servo is attached to
 * 
 * @return 0 if the operation was successful
*/
int servo_disable(const uint gpio_pin);

/**
 * Sets the position of the servo using the the duty cycle of the PWM signal.
 *
 * @param gpio_pin: the GPIO pin the servo is attached to
 * @param degree: the position in degree, a value within 0-180
 *
 * @return 0 if the operation was successful
*/
int servo_set(const uint gpio_pin, const uint16_t degree);

#endif