/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdbool.h>
#include "platform/sys.h"
#include "platform/time.h"

#include "ctrl/aircraft.h"
#include "ctrl/throttle.h"
#include "lib/parson.h"
#include "sys/configuration.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "test_throttle.h"

/**
 * Waits for a given number of seconds whilst updating the throttle.
 * @param s Number of seconds to wait
 */
static void wait_for(u32 s) {
    Timestamp wait = timestamp_in_ms(s * 1000);
    while (!timestamp_reached(&wait)) {
        throttle_update();
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
    if (!root) {
        return false;
    }
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
    if (aircraft_get_mode() != MODE_DIRECT) {
        return 403;
    }

    f32 *idle = &calibration.esc.detentIdle;
    f32 *mct = &calibration.esc.detentMct;
    f32 *max = &calibration.esc.detentMax;
    f32 t_idle = 4, t_mct = 2, t_max = 1;
    if (args) {
        if (!parse_args(args, &t_idle, &t_mct, &t_max)) {
            return 400;
        }
    }
    // Transition into thrust mode to set thrust percentages
    throttle_set_mode(THRMODE_THRUST);
    printpre("test", "setting IDLE (%.1f%%) for %.1fs", *idle, t_idle);
    throttle_set_target(*idle);
    wait_for((u32)(t_idle));
    printpre("test", "setting MCT (%.1f%%) for %.1fs", *mct, t_mct);
    throttle_set_target(*mct);
    wait_for((u32)(t_mct));
    printpre("test", "setting MAX (%.1f%%) for %.1fs", *max, t_max);
    throttle_set_target(*max);
    wait_for((u32)(t_max));
    throttle_set_target(0);
    return 200;
}
