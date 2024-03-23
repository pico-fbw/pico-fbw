#pragma once

#include "platform/int.h"

#define DEFAULT_SERVO_TEST                                                                                                     \
    { 110.f, 70.f, 90.f }               // Default degree amounts to move the servos to
#define DEFAULT_SERVO_TEST_PAUSE_MS 300 // Default pause between servo moves in milliseconds

/**
 * Enables servo control on a list of pins.
 * @param pins the array of servo GPIO pins
 * @param num_pins the number of pins
 */
void servo_enable(const u32 pins[], u32 num_pins);

/**
 * Sets the position of the servo using the the duty cycle of the PWM signal.
 * @param gpio_pin the GPIO pin the servo is attached to
 * @param degree the position in degrees, within 0-180
 */
void servo_set(u32 pin, float degree);

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
void servo_test(u32 servos[], u32 num_servos, float degrees[], u32 num_degrees, u32 pause_between_moves_ms);

/**
 * Gets the GPIO pins and number of pins designated as servos in the config.
 * @param pins array of at least 3 elements to fill with pins
 * @param num_pins pointer to the number of pins
 */
void servo_get_pins(u32 *servos, u32 *num_servos);
