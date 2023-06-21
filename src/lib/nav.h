#ifndef math_h
#define math_h

#define M_PI 3.14159265358979323846

/**
 * Calculates the bearing between two points.
 * @param lat1 Latitude of the first point.
 * @param lon1 Longitude of the first point.
 * @param lat2 Latitude of the second point.
 * @param lon2 Longitude of the second point.
 * @return Bearing in degrees.
*/
double calculateBearing(double latA, double lonA, double latB, double lonB);

#endif // math_h
