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

#include "../../../../modes/modes.h"

#include "test_esc.h"

uint api_test_esc(const char *cmd, const char *args) {
    if (aircraft.getMode() == MODE_DIRECT) {
        uint t_idle = 5, t_mct = 2, t_max = 1;
        if (args) {
            if (sscanf(args, "%d %d %d", &t_idle, &t_mct, &t_max) < 3) return 400;
        }
        printf("[ESC] setting idle thrust for %ds\n", t_idle);
        esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint)flash.control[CONTROL_THROTTLE_DETENT_IDLE]);
        absolute_time_t stop = make_timeout_time_ms(t_idle);
        while (!time_reached((stop))) watchdog_update();
        printf("[ESC] setting MCT for %ds\n", t_mct);
        esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint)flash.control[CONTROL_THROTTLE_DETENT_MCT]);
        stop = make_timeout_time_ms(t_mct);
        while (!time_reached(stop)) watchdog_update();
        printf("[ESC] setting MAX for %ds\n", t_mct);
        esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint)flash.control[CONTROL_THROTTLE_DETENT_MAX]);
        stop = make_timeout_time_ms(t_max);
        while (!time_reached(stop)) watchdog_update();
        esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint)flash.control[CONTROL_THROTTLE_DETENT_IDLE]);
    } else return 403;
    return 200;
}
