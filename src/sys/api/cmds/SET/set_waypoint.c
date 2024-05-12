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

#define DEFAULT_ALT -5
#define DEFAULT_SPEED -5
#define DEFAULT_DROP 0

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
    if (altVal && json_value_get_type(altVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    JSON_Value *spdVal = json_object_get_value(obj, "spd");
    if (spdVal && json_value_get_type(spdVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    JSON_Value *drpVal = json_object_get_value(obj, "drp");
    if (drpVal && json_value_get_type(drpVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    wpt->lat = (f64)json_value_get_number(latVal);
    wpt->lng = (f64)json_value_get_number(lngVal);
    wpt->alt = altVal ? (i32)json_value_get_number(altVal) : DEFAULT_ALT;
    wpt->speed = spdVal ? (f32)json_value_get_number(spdVal) : DEFAULT_SPEED;
    wpt->drop = drpVal ? (i32)json_value_get_number(drpVal) : DEFAULT_DROP;
    json_value_free(root);
    return true;
}

// {"lat":number,"lng":number}
// {"lat":number,"lng":number,"alt":number}
// {"lat":number,"lng":number,"alt":number,"spd":number}
// {"lat":number,"lng":number,"alt":number,"spd":number,"drp":number}

i32 api_set_waypoint(const char *args) {
    if (aircraft.mode != MODE_AUTO)
        return 403;

    Waypoint wpt;
    if (!parse_args(args, &wpt) || !waypoint_is_valid(&wpt))
        return 400;
    auto_set(wpt, callback_intercept);
    return 202;
}
