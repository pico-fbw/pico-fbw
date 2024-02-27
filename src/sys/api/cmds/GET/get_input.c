/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include "io/receiver.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "get_input.h"

i32 api_get_input(const char *cmd, const char *args) {
    printraw("{\"ail\":%f,\"elev\":%f,\"rud\":%f,\"thr\":%f,\"switch\":%f}\n", receiver_get(config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEG),
           receiver_get(config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEG), receiver_get(config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEG),
           receiver_get(config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_ESC), receiver_get(config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEG));
    return -1;
}
