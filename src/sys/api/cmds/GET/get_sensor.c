/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdlib.h>

#include "io/aahrs.h"
#include "io/gps.h"

#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "get_sensor.h"

i32 api_get_sensor(const char *cmd, const char *args) {
    // Prepare the JSON output based on sensor type
    switch (atoi(args)) {
        case 1: // IMU only
            if (aircraft.AAHRSSafe) {
                printraw("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", aahrs.roll, aahrs.pitch, aahrs.yaw);
            } else {
                printraw("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}]}\n");
            }
            break;
        case 2: // GPS only
            if ((GPSCommandType)config.sensors[SENSORS_GPS_COMMAND_TYPE] == GPS_COMMAND_TYPE_NONE) return 403;
            if (aircraft.GPSSafe) {
                printraw("{\"gps\":[{\"lat\":%.10f,\"lng\":%.10f,\"alt\":%d,\"speed\":%.4f,\"track\":%.4f}]}\n",
                        gps.lat, gps.lng, gps.alt, gps.speed, gps.track);
            } else {
                printraw("{\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"speed\":null,\"track\":null}]}\n");
            }
            break;
        case 0: // All sensors
        default:
            if ((GPSCommandType)config.sensors[SENSORS_GPS_COMMAND_TYPE] == GPS_COMMAND_TYPE_NONE) return 403;
            if (aircraft.AAHRSSafe && aircraft.GPSSafe) {
                printraw("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                       "\"gps\":[{\"lat\":%.10f,\"lng\":%.10f,\"alt\":%d,\"speed\":%.4f,\"track\":%.4f}]}\n",
                       aahrs.roll, aahrs.pitch, aahrs.yaw, gps.lat, gps.lng, gps.alt, gps.speed, gps.track);
            } else if (aircraft.AAHRSSafe) {
                printraw("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],"
                       "\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"speed\":null,\"track\":null}]}\n",
                       aahrs.roll, aahrs.pitch, aahrs.yaw);
            } else if (aircraft.GPSSafe) {
                printraw("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}],"
                       "\"gps\":[{\"lat\":%.10f,\"lng\":%.10f,\"alt\":%d,\"speed\":%.4f,\"track\":%.4f}]}\n",
                       gps.lat, gps.lng, gps.alt, gps.speed, gps.track);
            } else {
                printraw("{\"imu\":[{\"roll\":null,\"pitch\":null,\"yaw\":null}],"
                       "\"gps\":[{\"lat\":null,\"lng\":null,\"alt\":null,\"speed\":null,\"track\":null}]}\n");
            }
            break;
    }
    return -1;
}
