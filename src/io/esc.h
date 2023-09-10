#ifndef __ESC_H
#define __ESC_H

/**
 * Sets the speed of the ESC using the the duty cycle of the PWM signal.
 * @param gpio_pin the GPIO pin the ESC is attached to
 * @param speed the speed to set, a value between 0 and 100
*/
void esc_set(const uint gpio_pin, const uint16_t speed);

/**
 * Enables ESC control on a certain pin.
 * @param gpio_pin the GPIO pin the ESC is attached to
 * @return 0 if the operation was successful, 1 if the frequency was too small, and 2 if the frequency was too large.
*/
uint esc_enable(const uint gpio_pin);

/**
 * Disables ESC control on a certain pin.
 * @param gpio_pin the GPIO pin the ESC is attached to
*/
void esc_disable(const uint gpio_pin);

#endif // __ESC_H
