/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/types.h"

#include "../../../../io/flash.h"
#include "../../../../io/gps.h"

#include "../../../../modes/aircraft.h"

#include "test_gps.h"

int api_test_gps(const char *cmd, const char *args) {
    if ((GPSCommandType)flash.sensors[SENSORS_GPS_COMMAND_TYPE] == GPS_COMMAND_TYPE_NONE) return 403;
    if (!aircraft.GPSSafe) return 500;
    printf("[api] dumping GPS data, check for validity!\n"
           "==================================\n"
           "lat::%.10f, lng::%.10f, alt::%d, speed::%f, track::%f, "
           "pdop::%f, hdop::%f, vdop::%f, altOffset::%d, altOffset_calibrated::%d\n"
           "==================================\n",
           gps.lat, gps.lng, gps.alt, gps.speed, gps.track,
           gps.pdop, gps.hdop, gps.vdop, gps.altOffset, gps.altOffset_calibrated);
    return 200;
}
