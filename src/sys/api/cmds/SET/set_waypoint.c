/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdbool.h>

#include "lib/parson.h"

#include "modes/aircraft.h"
#include "modes/auto.h"

#include "sys/flightplan.h"
#include "sys/print.h"

#include "set_waypoint.h"

// Callback for when the created Waypoint is intercepted
static void callback_intercept() {
    printraw("pico-fbw WPTINTC\n");
}

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @param wpt pointer to store the waypoint
 * @return true if the arguments were parsed successfully
 */
static bool parse_args(const char *args, Waypoint *wpt) {
    JSON_Value *root = json_parse_string(args);
    if (!root)
        return false;
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return false;
    }
    JSON_Value *latVal = json_object_get_value(obj, "lat");
    if (!latVal || json_value_get_type(latVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    JSON_Value *lngVal = json_object_get_value(obj, "lng");
    if (!lngVal || json_value_get_type(lngVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    JSON_Value *altVal = json_object_get_value(obj, "alt");
    if (!altVal || json_value_get_type(altVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    JSON_Value *speedVal = json_object_get_value(obj, "speed");
    if (!speedVal || json_value_get_type(speedVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    JSON_Value *dropVal = json_object_get_value(obj, "drop");
    if (!dropVal || json_value_get_type(dropVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    wpt->lat = (f64)json_value_get_number(latVal);
    wpt->lng = (f64)json_value_get_number(lngVal);
    wpt->alt = (i32)json_value_get_number(altVal);
    wpt->speed = (f32)json_value_get_number(speedVal);
    wpt->drop = (i32)json_value_get_number(dropVal);
    json_value_free(root);
    return true;
}

// {"lat":number,"lng":number,"alt":number,"speed":number,"drop":number}

i32 api_set_waypoint(const char *args) {
    if (aircraft.mode != MODE_AUTO)
        return 403;

    Waypoint wpt;
    if (!parse_args(args, &wpt) || !waypoint_is_valid(&wpt))
        return 400;
    auto_set(wpt, callback_intercept);
    return 202;
}
