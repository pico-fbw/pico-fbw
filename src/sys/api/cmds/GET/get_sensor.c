/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/types.h"

#include "../../../../io/aahrs.h"
#include "../../../../io/flash.h"
#include "../../../../io/gps.h"

#include "get_sensor.h"

uint api_get_sensor(const char *cmd, const char *args) {
    GPS gps = { 0, 0, 0, 0, 0 };
    if ((GPSCommandType)flash.sensors[SENSORS_GPS_COMMAND_TYPE] != GPS_COMMAND_TYPE_NONE) {
        gps = gps_getData();
    }

    // Prepare the JSON output based on sensor type
    switch (atoi(args)) {
        case 1: // IMU only
            if (aahrs.roll != INFINITY) {
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", aahrs.roll, aahrs.pitch, aahrs.yaw);
            } else {
                printf("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}]}\n");
            }
            break;
        case 2: // GPS only
            if ((GPSCommandType)flash.sensors[SENSORS_GPS_COMMAND_TYPE] == GPS_COMMAND_TYPE_NONE) return 501;
            if (gps.lat != INFINITY) {
                printf("{\"gps\":[{\"lat\":%Lf,\"lng\":%Lf,\"alt\":%d,\"spd\":%f,\"trk\":%f}]}\n",
                       gps.lat, gps.lng, gps.alt, gps.spd, gps.trk_true);
            } else {
                printf("{\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"spd\":null,\"trk\":null}]}\n");
            }
            break;
        case 0: // All sensors
        default:
            if (aahrs.roll != INFINITY) {
                if (gps.lat != INFINITY) {
                    printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                           "\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f,\"trk\":%f}]}\n",
                           aahrs.roll, aahrs.pitch, aahrs.yaw, gps.lat, gps.lng, gps.alt, gps.spd, gps.trk_true);
                } else {
                    printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                           "\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"spd\":null,\"trk\":null}]}\n",
                           aahrs.roll, aahrs.pitch, aahrs.yaw);
                }
            } else {
                return 204;
            }
            break;
    }
    return -1;
}
