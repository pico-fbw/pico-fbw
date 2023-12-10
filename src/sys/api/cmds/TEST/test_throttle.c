/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/watchdog.h"

#include "../../../../io/flash.h"
#include "../../../../io/platform.h"

#include "../../../../modes/modes.h"

#include "../../../../sys/throttle.h"

#include "test_throttle.h"

/**
 * Waits for a given number of seconds whilst updating the throttle.
 * @param s Number of seconds to wait
*/
static void waitFor(uint32_t s) {
    absolute_time_t wait = make_timeout_time_ms(s * 1000);
    while (!time_reached(wait)) {
        throttle.update();
        watchdog_update();
    }
}

uint api_test_throttle(const char *cmd, const char *args) {
    if (aircraft.mode == MODE_DIRECT) {
        float *idle = &flash.control[CONTROL_THROTTLE_DETENT_IDLE];
        float *mct = &flash.control[CONTROL_THROTTLE_DETENT_MCT];
        float *max = &flash.control[CONTROL_THROTTLE_DETENT_MAX];
        float t_idle = 4, t_mct = 2, t_max = 1;
        if (args)
            if (sscanf(args, "%f %f %f", &t_idle, &t_mct, &t_max) < 3) return 400;
        // Transition into thrust mode to set thrust percentages
        throttle.mode = THRMODE_THRUST;
        printf("[api] setting idle thrust (%.1f%%) for %.1fs\n", *idle, t_idle);
        throttle.target = *idle;
        waitFor((uint32_t)(t_idle));
        printf("[api] setting MCT (%.1f%%) for %.1fs\n", *mct, t_mct);
        throttle.target = *mct;
        waitFor((uint32_t)(t_mct));
        printf("[api] setting MAX thrust (%.1f%%) for %.1fs\n", *max, t_max);
        throttle.target = *max;
        waitFor((uint32_t)(t_max));
        throttle.target = 0;
    } else return 403;
    return 200;
}
