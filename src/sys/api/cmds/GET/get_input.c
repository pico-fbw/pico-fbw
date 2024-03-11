/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "io/receiver.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "get_input.h"

// FIXME: this command should account for the current control mode (and not simply try and read from uninitialized pins)

i32 api_get_input(const char *args) {
    printraw("{\"ail\":%f,\"elev\":%f,\"rud\":%f,\"thr\":%f,\"switch\":%f}\n",
             receiver_get(config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE),
             receiver_get(config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE),
             receiver_get(config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE),
             receiver_get(config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT),
             receiver_get(config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEGREE));
    return -1;
    (void)args;
}
