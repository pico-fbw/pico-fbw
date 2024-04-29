/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>

#include "nav.h"

#define EARTH_RADIUS_KM 6371                    // Earth's radius in kilometers
#define EARTH_RADIUS_M (EARTH_RADIUS_KM * 1000) // Earth's radius in meters

f64 calculate_bearing(f64 latA, f64 lonA, f64 latB, f64 lonB) {
    f64 thetaA = radians(latA);
    f64 thetaB = radians(latB);
    f64 deltaL = radians(lonB - lonA);

    f64 X = cos(thetaB) * sin(deltaL);
    f64 Y = cos(thetaA) * sin(thetaB) - sin(thetaA) * cos(thetaB) * cos(deltaL);

    f64 bearing = atan2(X, Y) * 180.0 / M_PI;
    // Map to a heading instead of a degree value
    if (bearing <= 0) {
        bearing += 360;
    }
    return bearing;
}

f64 calculate_distance(f64 latA, f64 lonA, f64 latB, f64 lonB) {
    f64 thetaA = radians(latA);
    f64 thetaB = radians(latB);
    f64 deltaT = radians(latB - latA);
    f64 deltaL = radians(lonB - lonA);

    f64 a = sin(deltaT / 2) * sin(deltaT / 2) +
               cos(thetaA) * cos(thetaB) * sin(deltaL / 2) * sin(deltaL / 2);
    f64 c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS_M * c;
}
