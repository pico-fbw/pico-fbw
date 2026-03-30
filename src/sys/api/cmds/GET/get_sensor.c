/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <string.h>
#include "platform/defs.h"

#include "ctrl/aircraft.h"
#include "io/gps.h"
#include "io/imu.h"
#include "lib/parson.h"
#include "sys/configuration.h"

#include "get_sensor.h"

typedef enum SensorData {
    DATA_INVALID,
    DATA_ALL,
    DATA_GPS,
    DATA_IMU,
} SensorData;

/**
 * @return JSON object with GPS data, or NULL on error
 */
static JSON_Value *create_gps_obj() {
    JSON_Value *gpsObj = json_value_init_object();
    if (!gpsObj) {
        return NULL;
    }
    JSON_Object *obj = json_value_get_object(gpsObj);
    if (aircraft_is_gps_safe() && gps.is_supported()) {
        json_object_set_number(obj, "lat", gps.lat);
        json_object_set_number(obj, "lng", gps.lng);
        json_object_set_number(obj, "alt", gps.alt);
        json_object_set_number(obj, "speed", gps.speed);
        json_object_set_number(obj, "track", gps.track);
        json_object_set_number(obj, "pdop", gps.pdop);
        json_object_set_number(obj, "hdop", gps.hdop);
        json_object_set_number(obj, "vdop", gps.vdop);
        json_object_set_number(obj, "sats", gps.sats);
    } else {
        json_object_set_null(obj, "lat");
        json_object_set_null(obj, "lng");
        json_object_set_null(obj, "alt");
        json_object_set_null(obj, "speed");
        json_object_set_null(obj, "track");
        json_object_set_null(obj, "pdop");
        json_object_set_null(obj, "hdop");
        json_object_set_null(obj, "vdop");
        json_object_set_null(obj, "sats");
    }
    return gpsObj;
}

/**
 * @return JSON object with IMU data, or NULL on error
 */
static JSON_Value *create_imu_obj() {
    JSON_Value *imuObj = json_value_init_object();
    if (!imuObj) {
        return NULL;
    }
    JSON_Object *obj = json_value_get_object(imuObj);
    if (aircraft_is_imu_safe()) {
        json_object_set_number(obj, "roll", imu.roll);
        json_object_set_number(obj, "pitch", imu.pitch);
        json_object_set_number(obj, "yaw", imu.yaw);
        json_object_set_number(obj, "roll_rate", imu.rollRate);
        json_object_set_number(obj, "pitch_rate", imu.pitchRate);
        json_object_set_number(obj, "yaw_rate", imu.yawRate);
        json_object_set_number(obj, "accel_x", imu.accel[0]);
        json_object_set_number(obj, "accel_y", imu.accel[1]);
        json_object_set_number(obj, "accel_z", imu.accel[2]);
    } else {
        json_object_set_null(obj, "roll");
        json_object_set_null(obj, "pitch");
        json_object_set_null(obj, "yaw");
        json_object_set_null(obj, "roll_rate");
        json_object_set_null(obj, "pitch_rate");
        json_object_set_null(obj, "yaw_rate");
        json_object_set_null(obj, "accel_x");
        json_object_set_null(obj, "accel_y");
        json_object_set_null(obj, "accel_z");
    }
    return imuObj;
}

static SensorData parse_args(const char *args) {
    JSON_Value *root = json_parse_string(args);
    if (!root) {
        return DATA_INVALID;
    }
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
    if (strcasecmp(data, "all") == 0) {
        ret = DATA_ALL;
    } else if (strcasecmp(data, "gps") == 0) {
        ret = DATA_GPS;
    } else if (strcasecmp(data, "imu") == 0) {
        ret = DATA_IMU;
    }
    json_value_free(root);
    return ret;
}

// Input:
// {"data":"all|gps|imu"}

// Output (for data="all"):
// {
//  "gps":{"lat":number|null,"lng":number|null,"alt":number|null,"speed":number|null,"track":number|null,
//         "pdop":number|null,"hdop":number|null,"vdop":number|null,"sats":number|null},
//  "imu":{"roll":number|null,"pitch":number|null,"yaw":number|null,"roll_rate":number|null,"pitch_rate":number|null,
//           "yaw_rate":number|null,"accel_x":number|null,"accel_y":number|null,"accel_z":number|null}
// }

i32 api_get_sensor(const char *in, char **out) {
    // Parse args to determine the sensor data we should return
    SensorData data = parse_args(in);
    if (data == DATA_INVALID) {
        return 400;
    }

    // Generate all response data, regardless of the request
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);

    JSON_Value *gpsObj = create_gps_obj();
    JSON_Value *imuObj = create_imu_obj();
    if (!gpsObj || !imuObj) {
        return 500;
    }

    // Now, include response data selectively based on the request
    switch (data) {
        default:
        case DATA_ALL:
            json_object_set_value(obj, "gps", gpsObj);
            json_object_set_value(obj, "imu", imuObj);
            break;
        case DATA_GPS:
            if (!gps.is_supported()) {
                json_value_free(gpsObj);
                json_value_free(imuObj);
                json_value_free(root);
                return 403;
            }
            json_object_set_value(obj, "gps", gpsObj);
            break;
        case DATA_IMU:
            json_object_set_value(obj, "imu", imuObj);
            break;
    }
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
    return 200;
}
