#include <math.h>

double calculateBearing(double latA, double lonA, double latB, double lonB) {
    double thetaA = latA * M_PI / 180.0;
    double thetaB = latB * M_PI / 180.0;
    double deltaL = (lonB - lonA) * M_PI / 180.0;

    double X = cos(thetaB) * sin(deltaL);
    double Y = cos(thetaA) * sin(thetaB) - sin(thetaA) * cos(thetaB) * cos(deltaL);

    double bearing = atan2(X, Y);
    return bearing * 180.0 / M_PI;
}
