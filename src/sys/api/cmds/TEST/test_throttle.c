/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdbool.h>
#include "platform/sys.h"
#include "platform/time.h"

#include "lib/parson.h"

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
static void wait_for(u32 s) {
    Timestamp wait = timestamp_in_ms(s * 1000);
    while (!timestamp_reached(&wait)) {
        throttle.update();
        sys_periodic();
    }
}

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @param t_idle pointer to store idle thrust time
 * @param t_mct pointer to store MCT thrust time
 * @param t_max pointer to store max thrust time
 * @return true if parsing was successful
 */
static bool parse_args(const char *args, f32 *t_idle, f32 *t_mct, f32 *t_max) {
    JSON_Value *root = json_parse_string(args);
    if (!root)
        return false;
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return false;
    }
    *t_idle = (f32)json_object_get_number(obj, "idle");
    *t_mct = (f32)json_object_get_number(obj, "mct");
    *t_max = (f32)json_object_get_number(obj, "max");
    json_value_free(root);
    return (*t_idle > 0 && *t_mct > 0 && *t_max > 0);
}

// {"idle":number,"mct":number,"max":number}

i32 api_test_throttle(const char *args) {
    if (aircraft.mode != MODE_DIRECT)
        return 403;

    f32 *idle = &calibration.esc[ESC_DETENT_IDLE];
    f32 *mct = &calibration.esc[ESC_DETENT_MCT];
    f32 *max = &calibration.esc[ESC_DETENT_MAX];
    f32 t_idle = 4, t_mct = 2, t_max = 1;
    if (args)
        if (!parse_args(args, &t_idle, &t_mct, &t_max))
            return 400;
    // Transition into thrust mode to set thrust percentages
    throttle.mode = THRMODE_THRUST;
    printpre("test", "setting IDLE (%.1f%%) for %.1fs", *idle, t_idle);
    throttle.target = *idle;
    wait_for((u32)(t_idle));
    printpre("test", "setting MCT (%.1f%%) for %.1fs", *mct, t_mct);
    throttle.target = *mct;
    wait_for((u32)(t_mct));
    printpre("test", "setting MAX (%.1f%%) for %.1fs", *max, t_max);
    throttle.target = *max;
    wait_for((u32)(t_max));
    throttle.target = 0;
    return 200;
}
