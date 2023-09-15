#ifndef __SERVO_H
#define __SERVO_H

#include "../config.h"

#if defined(CONTROL_3AXIS)
    #define SERVO_PINS {SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN}
    #define NUM_SERVOS 3
#elif defined(CONTROL_FLYINGWING)
    #define SERVO_PINS {SERVO_ELEVON_L_PIN, SERVO_ELEVON_R_PIN}
    #define NUM_SERVOS 2
#endif

#define DEFAULT_SERVO_TEST {110, 70, 90} // Default degree amounts to move the servos to during the POST
#define NUM_DEFAULT_SERVO_TEST 3
#define DEFAULT_SERVO_TEST_PAUSE_MS 250 // Default pause between servo moves in milliseconds

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
 * "Tests" a list of servos by moving them to a list of degree positions.
 * @param servos the array of servo GPIO pins (already enabled)
 * @param num_servos the number of servos
 * @param degrees the array of degree positions
 * @param num_degrees the number of degree positions
 * @param pause_between_moves_ms the pause between servo moves in milliseconds
 * @note If num_servos = 0, the servos from the config will be tested.
 * You also may want to add 90 at the end of the list to ensure that the servos return to a neutral position.
*/
void servo_test(const uint servos[], const uint num_servos, const uint16_t degrees[], const uint num_degrees, const uint pause_between_moves_ms);

/**
 * Disables servo control on a certain pin.
 * @param gpio_pin the GPIO pin the servo is attached to
*/
void servo_disable(const uint gpio_pin);

#endif // __SERVO_H
