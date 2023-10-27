/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/types.h"

#include "../../../../io/imu.h"
#include "../../../../io/gps.h"

#include "../../../config.h"

#include "get_sensor.h"

uint api_get_sensor(const char *cmd, const char *args) {
    Angles imu = imu_getAngles();
    GPS gps = { 0, 0, 0, 0, 0 };
    if (config.sensors.gpsCommandType != GPS_COMMAND_TYPE_NONE) {
        gps = gps_getData();
    }

    // Prepare the JSON output based on sensor type
    switch (atoi(args)) {
        case 1: // IMU only
            if (imu.roll == INFINITY) {
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", imu.roll, imu.pitch, imu.yaw);
            } else {
                printf("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}]}\n");
            }
            break;
        case 2: // GPS only
            if (!config.sensors.gpsCommandType != GPS_COMMAND_TYPE_NONE) {
                return 501;
            }
            if (gps.lat != INFINITY) {
                printf("{\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f,\"trk\":%f}]}\n",
                       gps.lat, gps.lng, gps.alt, gps.spd, gps.trk_true);
            } else {
                printf("{\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"spd\":null,\"trk\":null}]}\n");
            }
            break;
        case 0: // All sensors
        default:
            if (imu.roll != INFINITY) {
                if (gps.lat != INFINITY) {
                    printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                           "\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f,\"trk\":%f}]}\n",
                           imu.roll, imu.pitch, imu.yaw, gps.lat, gps.lng, gps.alt, gps.spd, gps.trk_true);
                } else {
                    printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                           "\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"spd\":null,\"trk\":null}]}\n",
                           imu.roll, imu.pitch, imu.yaw);
                }
            } else {
                return 204;
            }
            break;
    }
    return -1;
}
