/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include "io/gps.h"

#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "test_gps.h"

i32 api_test_gps(const char *cmd, const char *args) {
    if ((GPSCommandType)config.sensors[SENSORS_GPS_COMMAND_TYPE] == GPS_COMMAND_TYPE_NONE) return 403;
    if (!aircraft.GPSSafe) return 500;
    printraw("[test] dumping GPS data, check for validity!\n"
           "==================================\n"
           "lat::%.10f, lng::%.10f, alt::%d, speed::%f, track::%f, "
           "pdop::%f, hdop::%f, vdop::%f, altOffset::%d, altOffsetCalibrated::%d\n"
           "==================================\n",
           gps.lat, gps.lng, gps.alt, gps.speed, gps.track,
           gps.pdop, gps.hdop, gps.vdop, gps.altOffset, gps.altOffsetCalibrated);
    return 200;
}
