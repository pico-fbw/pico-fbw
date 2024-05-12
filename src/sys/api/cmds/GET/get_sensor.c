/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/adc.h"
#include "platform/defs.h"

#include "io/aahrs.h"
#include "io/gps.h"

#include "lib/parson.h"

#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "get_sensor.h"

typedef enum SensorData {
    DATA_INVALID,
    DATA_ALL,
    DATA_AAHRS,
    DATA_GPS,
    DATA_BATT,
} SensorData;

/**
 * @return JSON object with AAHRS data, or NULL on error
 */
static JSON_Value *create_aahrs_obj() {
    JSON_Value *aahrsObj = json_value_init_object();
    if (!aahrsObj)
        return NULL;
    JSON_Object *obj = json_value_get_object(aahrsObj);
    if (aircraft.aahrsSafe) {
        json_object_set_number(obj, "roll", aahrs.roll);
        json_object_set_number(obj, "pitch", aahrs.pitch);
        json_object_set_number(obj, "yaw", aahrs.yaw);
    } else {
        json_object_set_null(obj, "roll");
        json_object_set_null(obj, "pitch");
        json_object_set_null(obj, "yaw");
    }
    return aahrsObj;
}

/**
 * @return JSON object with GPS data, or NULL on error
 */
static JSON_Value *create_gps_obj() {
    JSON_Value *gpsObj = json_value_init_object();
    if (!gpsObj)
        return NULL;
    JSON_Object *obj = json_value_get_object(gpsObj);
    if (aircraft.gpsSafe && gps.is_supported()) {
        json_object_set_number(obj, "lat", gps.lat);
        json_object_set_number(obj, "lng", gps.lng);
        json_object_set_number(obj, "alt", gps.alt);
        json_object_set_number(obj, "speed", gps.speed);
        json_object_set_number(obj, "track", gps.track);
    } else {
        json_object_set_null(obj, "lat");
        json_object_set_null(obj, "lng");
        json_object_set_null(obj, "alt");
        json_object_set_null(obj, "speed");
        json_object_set_null(obj, "track");
    }
    return gpsObj;
}

/**
 * @return JSON array with battery data, or NULL on error
 */
static JSON_Value *create_batt_arr() {
#if PLATFORM_SUPPORTS_ADC
    JSON_Value *battArr = json_value_init_array();
    if (!battArr)
        return NULL;
    JSON_Array *arr = json_value_get_array(battArr);
    for (u32 i = 0; i < ADC_NUM_CHANNELS; i++) {
        json_array_append_number(arr, adc_read_raw(ADC_PINS[i]));
    }
    return battArr;
#else
    return json_value_init_array();
#endif
}

static SensorData parse_args(const char *args) {
    JSON_Value *root = json_parse_string(args);
    if (!root)
        return DATA_INVALID;
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return DATA_INVALID;
    }
    const char *data = json_object_get_string(obj, "data");
    if (!data) {
        json_value_free(root);
        return DATA_INVALID;
    }
    SensorData ret = DATA_INVALID;
    if (strcasecmp(data, "all") == 0)
        ret = DATA_ALL;
    else if (strcasecmp(data, "aahrs") == 0)
        ret = DATA_AAHRS;
    else if (strcasecmp(data, "gps") == 0)
        ret = DATA_GPS;
    else if (strcasecmp(data, "batt") == 0)
        ret = DATA_BATT;
    json_value_free(root);
    return ret;
}

// Input:
// {"data":"all|aahrs|gps|batt"}

// Output (for data="all", note that "batt" may not exist and may have a different length):
// {
//  "aahrs":{"roll":number|null,"pitch":number|null,"yaw":number|null},
//  "gps":{"lat":number|null,"lng":number|null,"alt":number|null,"speed":number|null,"track":number|null},
//  "batt":"batt":[number,...]
// }

i32 api_get_sensor(const char *args) {
    // Parse args to determine the sensor data we should return
    SensorData data = parse_args(args);
    if (data == DATA_INVALID)
        return 400;

    // Generate all response data, regardless of the request
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    JSON_Value *aahrsObj = create_aahrs_obj();
    JSON_Value *gpsObj = create_gps_obj();
    JSON_Value *battArr = create_batt_arr();
    if (!aahrsObj || !gpsObj || !battArr)
        return 500;

    // Now, include response data selectively based on the request
    switch (data) {
        default:
        case DATA_ALL:
            json_object_set_value(obj, "aahrs", aahrsObj);
            json_object_set_value(obj, "gps", gpsObj);
#if PLATFORM_SUPPORTS_ADC
            json_object_set_value(obj, "batt", battArr);
#endif
            break;
        case DATA_AAHRS:
            json_object_set_value(obj, "aahrs", aahrsObj);
            break;
        case DATA_GPS:
            if (!gps.is_supported()) {
                json_value_free(battArr);
                json_value_free(gpsObj);
                json_value_free(aahrsObj);
                json_value_free(root);
                return 403;
            }
            json_object_set_value(obj, "gps", gpsObj);
            break;
        case DATA_BATT:
#if PLATFORM_SUPPORTS_ADC
            json_object_set_value(obj, "batt", battArr);
#else
            json_value_free(battArr);
            json_value_free(gpsObj);
            json_value_free(aahrsObj);
            json_value_free(root);
            return 403;
#endif
            break;
    }
    char *serialized = json_serialize_to_string(root);
    printraw("%s\n", serialized);
    json_free_serialized_string(serialized);
    json_value_free(root);
    return -1;
}
