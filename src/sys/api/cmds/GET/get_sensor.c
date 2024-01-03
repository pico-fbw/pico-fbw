/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include <stdlib.h>

#include "../../../../io/aahrs.h"
#include "../../../../io/flash.h"
#include "../../../../io/gps.h"

#include "../../../../modes/aircraft.h"

#include "get_sensor.h"

int api_get_sensor(const char *cmd, const char *args) {
    // Prepare the JSON output based on sensor type
    switch (atoi(args)) {
        case 1: // IMU only
            if (aircraft.AAHRSSafe) {
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", aahrs.roll, aahrs.pitch, aahrs.yaw);
            } else {
                printf("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}]}\n");
            }
            break;
        case 2: // GPS only
            if ((GPSCommandType)flash.sensors[SENSORS_GPS_COMMAND_TYPE] == GPS_COMMAND_TYPE_NONE) return 403;
            if (aircraft.GPSSafe) {
                printf("{\"gps\":[{\"lat\":%.10f,\"lng\":%.10f,\"alt\":%d,\"speed\":%.4f,\"track\":%.4f}]}\n",
                        gps.lat, gps.lng, gps.alt, gps.speed, gps.track);
            } else {
                printf("{\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"speed\":null,\"track\":null}]}\n");
            }
            break;
        case 0: // All sensors
        default:
            if ((GPSCommandType)flash.sensors[SENSORS_GPS_COMMAND_TYPE] == GPS_COMMAND_TYPE_NONE) return 403;
            if (aircraft.AAHRSSafe && aircraft.GPSSafe) {
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                       "\"gps\":[{\"lat\":%.10f,\"lng\":%.10f,\"alt\":%d,\"speed\":%.4f,\"track\":%.4f}]}\n",
                       aahrs.roll, aahrs.pitch, aahrs.yaw, gps.lat, gps.lng, gps.alt, gps.speed, gps.track);
            } else if (aircraft.AAHRSSafe) {
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                       "\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"speed\":null,\"track\":null}]}\n",
                       aahrs.roll, aahrs.pitch, aahrs.yaw);
            } else if (aircraft.GPSSafe) {
                printf("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}],"
                       "\"gps\":[{\"lat\":%.10f,\"lng\":%.10f,\"alt\":%d,\"speed\":%.4f,\"track\":%.4f}]}\n",
                       gps.lat, gps.lng, gps.alt, gps.speed, gps.track);
            } else {
                printf("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}],"
                       "\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"speed\":null,\"track\":null}]}\n");
            }
            break;
    }
    return -1;
}
