/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdio.h>
#include "platform/sys.h"
#include "platform/time.h"

#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/print.h"
#include "sys/runtime.h"
#include "sys/throttle.h"

#include "test_throttle.h"

/**
 * Waits for a given number of seconds whilst updating the throttle.
 * @param s Number of seconds to wait
 */
static void waitFor(u32 s) {
    Timestamp wait = timestamp_in_ms(s * 1000);
    while (!timestamp_reached(&wait)) {
        throttle.update();
        sys_periodic();
    }
}

i32 api_test_throttle(const char *args) {
    if (aircraft.mode == MODE_DIRECT) {
        float *idle = &calibration.esc[ESC_DETENT_IDLE];
        float *mct = &calibration.esc[ESC_DETENT_MCT];
        float *max = &calibration.esc[ESC_DETENT_MAX];
        float t_idle = 4, t_mct = 2, t_max = 1;
        if (args)
            if (sscanf(args, "%f %f %f", &t_idle, &t_mct, &t_max) < 3)
                return 400;
        // Transition into thrust mode to set thrust percentages
        throttle.mode = THRMODE_THRUST;
        printraw("[test] setting idle thrust (%.1f%%) for %.1fs\n", *idle, t_idle);
        throttle.target = *idle;
        waitFor((u32)(t_idle));
        printraw("[test] setting MCT (%.1f%%) for %.1fs\n", *mct, t_mct);
        throttle.target = *mct;
        waitFor((u32)(t_mct));
        printraw("[test] setting MAX thrust (%.1f%%) for %.1fs\n", *max, t_max);
        throttle.target = *max;
        waitFor((u32)(t_max));
        throttle.target = 0;
    } else
        return 403;
    return 200;
}
