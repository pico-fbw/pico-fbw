/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/int.h"

#include "io/esc.h"
#include "io/receiver.h"
#include "io/servo.h"

#include "sys/configuration.h"

#include "direct.h"

/**
 * Quick note for future me/any other developers (if there ever are lol):
 * This mode is as low-latency as I can think to make it--the only way (I believe) to make it faster is to
 * reassign the GPIO pins being used for input to DIO instead of PIO and assign them back when switching modes, which
 * gave a basically unnoticeable decrease in latency and it's much more complicated, so I didn't bother to implement it.
 */

void direct_update() {
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            servo_set((u32)config.pins[PINS_SERVO_RUD], (u16)receiver_get((u32)config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEG));
            /* fall through */
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            servo_set((u32)config.pins[PINS_SERVO_AIL], (u16)receiver_get((u32)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEG));
            servo_set((u32)config.pins[PINS_SERVO_ELE], (u16)receiver_get((u32)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEG));
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            // TODO: flying wing mixing here
            servo_set((u32)config.pins[PINS_SERVO_AIL], (u16)receiver_get((u32)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEG));
            servo_set((u32)config.pins[PINS_SERVO_ELE], (u16)receiver_get((u32)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEG));
            break;
    }
    if (receiver_has_athr())
        esc_set((u32)config.pins[PINS_ESC_THROTTLE],
                (u16)receiver_get((u32)config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_ESC));
}
