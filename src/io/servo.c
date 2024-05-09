/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/pwm.h"
#include "platform/time.h"

#include "io/receiver.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"

#include "servo.h"

void servo_enable(const u32 pins[], u32 num_pins) {
    printpre("servo", "setting up %lu servos", num_pins);
    if (!pwm_setup_write(pins, num_pins, config.general[GENERAL_SERVO_HZ]))
        log_message(TYPE_FATAL, "Failed to enable PWM output!", 500, 0, true);
    for (u32 i = 0; i < num_pins; i++)
        servo_set(pins[i], 90.f); // Set initial position to 90 degrees
}

void servo_set(u32 pin, f32 degree) {
    // Ensure speed is within range 0-180deg
    degree = clampf(degree, 0.f, 180.f);
    // Almost all servos expect a pulsewidth of 500-2500μs (500μs is 0deg, 2500μs is 180deg)
    pwm_write_raw(pin, mapf(degree, 0.f, 180.f, 500.f, 2500.f));
}

void servo_test(u32 servos[], u32 num_servos, f32 degrees[], u32 num_degrees, u32 pause_between_moves_ms) {
    for (u32 d = 0; d < num_degrees; d++) {
        for (u32 s = 0; s < num_servos; s++) {
            if (servos[s] == (u32)config.pins[PINS_SERVO_BAY]) {
                // The drop servo will be set to the configured detents so as not to possibly break it
                if (d < (num_degrees / 2))
                    servo_set(servos[s], config.control[CONTROL_DROP_DETENT_OPEN]);
                else
                    servo_set(servos[s], config.control[CONTROL_DROP_DETENT_CLOSED]);
            } else {
                servo_set(servos[s], degrees[d]);
            }
        }
        sleep_ms_blocking(pause_between_moves_ms);
    }
}

void servo_get_pins(u32 *servos, u32 *num_servos) {
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            servos[0] = (u32)config.pins[PINS_SERVO_AIL];
            servos[1] = (u32)config.pins[PINS_SERVO_ELE];
            servos[2] = (u32)config.pins[PINS_SERVO_RUD];
            servos[3] = (u32)config.pins[PINS_SERVO_BAY];
            *num_servos = 4;
            break;
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            servos[0] = (u32)config.pins[PINS_SERVO_AIL];
            servos[1] = (u32)config.pins[PINS_SERVO_ELE];
            servos[2] = (u32)config.pins[PINS_SERVO_BAY];
            *num_servos = 3;
            break;
    }
}
