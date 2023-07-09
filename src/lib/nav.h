#ifndef __NAV_H
#define __NAV_H

#define EARTH_RADIUS_NM 3443.93  // Earth radius in nautical miles

/**
 * Calculates the bearing between two points.
 * @param latA Latitude of the first point.
 * @param lonA Longitude of the first point.
 * @param latB Latitude of the second point.
 * @param lonB Longitude of the second point.
 * @return Bearing in degrees.
*/
double calculateBearing(double latA, double lonA, double latB, double lonB);

/**
 * Calculates the distance between two points.
 * @param latA Latitude of the first point.
 * @param lonA Longitude of the first point.
 * @param latB Latitude of the second point.
 * @param lonB Longitude of the second point.
 * @return Distance in nautical mi(y)les.
*/
double calculateDistance(double latA, double lonA, double latB, double lonB);

#endif // __NAV_H
