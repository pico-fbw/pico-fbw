#include <math.h>

#include "nav.h"

/**
 * I'm not writing documentation for this.
*/
static inline double toRadians(double degrees) {
    return degrees * M_PI / 180.0;
}

double calculateBearing(double latA, double lonA, double latB, double lonB) {
    // Fancy trig woaaaah I definetly came up with this
    double thetaA = toRadians(latA);
    double thetaB = toRadians(latB);
    double deltaL = toRadians(lonB - lonA);

    double X = cos(thetaB) * sin(deltaL);
    double Y = cos(thetaA) * sin(thetaB) - sin(thetaA) * cos(thetaB) * cos(deltaL);

    double bearing = atan2(X, Y) * 180.0 / M_PI;
    // Map to a heading instead of a degree value
    if (bearing <= 0) {
        bearing += 360;
    }
    return bearing;
}

double calculateDistance(double latA, double lonA, double latB, double lonB) {
    double thetaA = toRadians(latA);
    double thetaB = toRadians(latB);
    double deltaTheta = toRadians(latB - latA);
    double deltaLamda = toRadians(lonB - lonA);

    double a = sin(deltaTheta / 2) * sin(deltaTheta / 2) +
               cos(thetaA) * cos(thetaB) *
               sin(deltaLamda / 2) * sin(deltaLamda / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS_NM * c;           
}
