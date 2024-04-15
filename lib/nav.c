/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include "platform/int.h"

#include "nav.h"

#define EARTH_RADIUS_KM 6371                    // Earth's radius in kilometers
#define EARTH_RADIUS_M (EARTH_RADIUS_KM * 1000) // Earth's radius in meters

double calculate_bearing(long double latA, long double lonA, long double latB, long double lonB) {
    long double thetaA = radians(latA);
    long double thetaB = radians(latB);
    long double deltaL = radians(lonB - lonA);

    double X = cos((double)thetaB) * sin((double)deltaL);
    double Y = cos((double)thetaA) * sin((double)thetaB) - sin((double)thetaA) * cos((double)thetaB) * cos((double)deltaL);

    double bearing = atan2(X, Y) * 180.0 / M_PI;
    // Map to a heading instead of a degree value
    if (bearing <= 0) {
        bearing += 360;
    }
    return bearing;
}

double calculate_distance(long double latA, long double lonA, long double latB, long double lonB) {
    long double thetaA = radians(latA);
    long double thetaB = radians(latB);
    long double deltaT = radians(latB - latA);
    long double deltaL = radians(lonB - lonA);

    double a = sin((double)deltaT / 2) * sin((double)deltaT / 2) +
               cos((double)thetaA) * cos((double)thetaB) * sin((double)deltaL / 2) * sin((double)deltaL / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS_M * c;
}
