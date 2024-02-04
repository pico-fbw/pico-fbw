/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <math.h>

#include "lib/nav.h"

static inline long double toRadians(long double degrees) {
    return degrees * M_PI / 180.0;
}

double calculateBearing(long double latA, long double lonA, long double latB, long double lonB) {
    long double thetaA = toRadians(latA);
    long double thetaB = toRadians(latB);
    long double deltaL = toRadians(lonB - lonA);

    double X = cos((double)thetaB) * sin((double)deltaL);
    double Y = cos((double)thetaA) * sin((double)thetaB) - sin((double)thetaA) * cos((double)thetaB) * cos((double)deltaL);

    double bearing = atan2(X, Y) * 180.0 / M_PI;
    // Map to a heading instead of a degree value
    if (bearing <= 0) {
        bearing += 360;
    }
    return bearing;
}

double calculateDistance(long double latA, long double lonA, long double latB, long double lonB) {
    long double thetaA = toRadians(latA);
    long double thetaB = toRadians(latB);
    long double deltaT = toRadians(latB - latA);
    long double deltaL = toRadians(lonB - lonA);

    double a = sin((double)deltaT / 2) * sin((double)deltaT / 2) +
               cos((double)thetaA) * cos((double)thetaB) *
               sin((double)deltaL / 2) * sin((double)deltaL / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS_M * c;           
}
