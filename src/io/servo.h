#ifndef PICO_SERVO_H
#define PICO_SERVO_H

/**
 * Enable Servo control.
 * @param gpio_pin: the GPIO pin the servo is attached to
 * 
 * @return 0 if the operation was successful
 * **/
int
servo_enable(const uint gpio_pin);

/**
 * Enable Servo control.
 * @param gpio_pin: the GPIO pin the servo is attached to
 * 
 * @return 0 if the operation was successful
 * **/
int
servo_disable(const uint gpio_pin);
/**
 * Sets the position of the servo using the the duty cycle of the PWM signal.
 *
 * @param gpio_pin: the GPIO pin the servo is attached to
 * @param degree: the position in degree, a value within 0..180
 *
 * @return 0 if the operation was successful
 * **/
int
servo_set_position(const uint gpio_pin, const uint16_t degree);

#endif