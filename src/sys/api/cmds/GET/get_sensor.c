/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include <stdlib.h>
#include "pico/types.h"

#include "../../../../io/aahrs.h"
#include "../../../../io/flash.h"
#include "../../../../io/gps.h"

#include "../../../../modes/modes.h"

#include "get_sensor.h"

uint api_get_sensor(const char *cmd, const char *args) {
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
                printf("{\"gps\":[{\"lat\":%lf,\"lng\":%lf,\"alt\":%d,\"spd\":%.4f,\"trk\":%.4f}]}\n",
                        gps.lat, gps.lng, gps.alt, gps.spd, gps.trk);
            } else {
                printf("{\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"spd\":null,\"trk\":null}]}\n");
            }
            break;
        case 0: // All sensors
        default:
            if ((GPSCommandType)flash.sensors[SENSORS_GPS_COMMAND_TYPE] == GPS_COMMAND_TYPE_NONE) return 403;
            if (aircraft.AAHRSSafe && aircraft.GPSSafe) {
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                       "\"gps\":[{\"lat\":%lf,\"lng\":%lf,\"alt\":%d,\"spd\":%.4f,\"trk\":%.4f}]}\n",
                       aahrs.roll, aahrs.pitch, aahrs.yaw, gps.lat, gps.lng, gps.alt, gps.spd, gps.trk);
            } else if (aircraft.AAHRSSafe) {
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                       "\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"spd\":null,\"trk\":null}]}\n",
                       aahrs.roll, aahrs.pitch, aahrs.yaw);
            } else if (aircraft.GPSSafe) {
                printf("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}],"
                       "\"gps\":[{\"lat\":%lf,\"lng\":%lf,\"alt\":%d,\"spd\":%.4f,\"trk\":%.4f}]}\n",
                       gps.lat, gps.lng, gps.alt, gps.spd, gps.trk);
            } else {
                printf("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}],"
                       "\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"spd\":null,\"trk\":null}]}\n");
            }
            break;
    }
    return -1;
}
