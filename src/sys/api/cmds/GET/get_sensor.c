/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/types.h"

#include "../../../../modes/flight.h"
#include "../../../../modes/modes.h"

#include "../../../config.h"

#include "get_sensor.h"

uint api_get_sensor(const char *cmd, const char *args) {
    // Sensors are not updated in direct mode
    if (getCurrentMode() != MODE_DIRECT) {
        switch (atoi(args)) {
            case 1: // IMU only
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw);
                break;
            case 2: // GPS only
                if (config.sensors.gpsEnabled) {
                    printf("{\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f,\"trk\":%f}]}\n",
                    gps.lat, gps.lng, gps.alt, gps.spd, gps.trk_true);
                } else {
                    return 501;
                }
                break;
            case 0: // All sensors
            default:
                if (config.sensors.gpsEnabled) {
                    printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f,\"trk\":%f}]}\n",
                    aircraft.roll, aircraft.pitch, aircraft.yaw, gps.lat, gps.lng, gps.alt, gps.spd, gps.trk_true);
                } else {
                    printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw);
                }
                break;
        }
        return 200;
    } else {
        return 403;
    }
}
