/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/watchdog.h"

#include "../../../../io/esc.h"
#include "../../../../io/flash.h"
#include "../../../../io/platform.h"

#include "../../../../modes/modes.h"

#include "test_esc.h"

uint api_test_esc(const char *cmd, const char *args) {
    if (aircraft.mode() == MODE_DIRECT) {
        float t_idle = 4, t_mct = 2, t_max = 1;
        if (args) {
            if (sscanf(args, "%f %f %f", &t_idle, &t_mct, &t_max) < 3) return 400;
        }
        printf("[api] setting idle thrust for %.1fs\n", t_idle);
        esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint16_t)flash.control[CONTROL_THROTTLE_DETENT_IDLE]);
        platform_sleep_ms((uint32_t)(t_idle * 1000));
        printf("[api] setting MCT for %.1fs\n", t_mct);
        esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint16_t)flash.control[CONTROL_THROTTLE_DETENT_MCT]);
        platform_sleep_ms((uint32_t)(t_mct * 1000));
        printf("[api] setting MAX for %.1fs\n", t_max);
        esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint16_t)flash.control[CONTROL_THROTTLE_DETENT_MAX]);
        platform_sleep_ms((uint32_t)(t_max * 1000));
        esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint16_t)flash.control[CONTROL_THROTTLE_DETENT_IDLE]);
    } else return 403;
    return 200;
}
