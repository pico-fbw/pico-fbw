/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include "platform/helpers.h"

#include "nav.h"

#define EARTH_RADIUS_KM 6371                    // Earth's radius in kilometers
#define EARTH_RADIUS_M (EARTH_RADIUS_KM * 1000) // Earth's radius in meters

f64 calculate_bearing(f64 latA, f64 lngA, f64 latB, f64 lngB) {
    f64 thetaA = radians(latA);
    f64 thetaB = radians(latB);
    f64 deltaL = radians(lngB) - radians(lngA);

    f64 y = sin(deltaL) * cos(thetaB);
    f64 x = cos(thetaA) * sin(thetaB) - sin(thetaA) * cos(thetaB) * cos(deltaL);

    return fmod((degrees(atan2(y, x)) + 360), 360);
}

f64 calculate_distance(f64 latA, f64 lngA, f64 latB, f64 lngB) {
    f64 thetaA = radians(latA);
    f64 thetaB = radians(latB);
    f64 deltaT = radians(latB - latA);
    f64 deltaL = radians(lngB - lngA);

    f64 a = sin(deltaT / 2) * sin(deltaT / 2) + cos(thetaA) * cos(thetaB) * sin(deltaL / 2) * sin(deltaL / 2);
    f64 c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS_M * c;
}
