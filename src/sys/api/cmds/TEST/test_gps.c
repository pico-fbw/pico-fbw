/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "ctrl/aircraft.h"

#include "io/gps.h"
#include "sys/configuration.h"
#include "sys/print.h"

#include "test_gps.h"

i32 api_test_gps(const char *args) {
    if (!gps.is_supported()) {
        return 403;
    }
    if (!aircraft_is_gps_safe()) {
        return 500;
    }
    printpre("test",
             "dumping GPS data, check for validity!\n"
             "==================================\n"
             "lat::%.10lf, lng::%.10lf, alt::%ld, speed::%f, track::%f, "
             "pdop::%f, hdop::%f, vdop::%f, altOffset::%ld, altOffsetCalibrated::%d\n"
             "==================================",
             gps.lat, gps.lng, gps.alt, gps.speed, gps.track, gps.pdop, gps.hdop, gps.vdop, gps.altOffset,
             gps.altOffsetCalibrated);
    return 200;
    (void)args;
}
