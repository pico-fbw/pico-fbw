/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "io/receiver.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "get_input.h"

i32 api_get_input(const char *args) {
    // Print only ail and ele first because they are guaranteed to be in all control modes
    printraw("{\"ail\":%f,\"elev\":%f,", receiver_get(config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE),
             receiver_get(config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE));
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
            printraw("\"rud\":%f,\"thr\":%f,\"switch\":%f}\n", receiver_get(config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE),
                     receiver_get(config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT),
                     receiver_get(config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEGREE));
            break;
        case CTRLMODE_3AXIS:
            printraw("\"rud\":%f,\"switch\":%f}\n", receiver_get(config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE),
                     receiver_get(config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEGREE));
            break;
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_FLYINGWING_ATHR:
            printraw("\"thr\":%f,\"switch\":%f}\n", receiver_get(config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT),
                     receiver_get(config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEGREE));
            break;
        case CTRLMODE_2AXIS:
        case CTRLMODE_FLYINGWING:
            printraw("\"switch\":%f}\n", receiver_get(config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEGREE));
            break;
    }
    return -1;
    (void)args;
}
