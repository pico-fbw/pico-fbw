#ifndef __ESC_H
#define __ESC_H

/**
 * Enables ESC control on a certain pin.
 * @param gpio_pin the GPIO pin the ESC is attached to
 * @return 0 if the operation was successful, 1 if the frequency was too small, and 2 if the frequency was too large.
*/
uint esc_enable(uint gpio_pin);

/**
 * Sets the speed of the ESC using the the duty cycle of the PWM signal.
 * @param gpio_pin the GPIO pin the ESC is attached to
 * @param speed the speed to set, a value between 0 and 100
*/
void esc_set(uint gpio_pin, uint16_t speed);

/**
 * Disables ESC control on a certain pin.
 * @param gpio_pin the GPIO pin the ESC is attached to
*/
void esc_disable(uint gpio_pin);

#endif // __ESC_H
