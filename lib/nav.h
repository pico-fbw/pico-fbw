#ifndef __NAV_H
#define __NAV_H

#define EARTH_RADIUS_KM 6371  // Earth's radius in kilometers
#define EARTH_RADIUS_M (EARTH_RADIUS_KM * 1000) // Earth's radius in meters

/**
 * Calculates the bearing between two points.
 * @param latA Latitude of the first point.
 * @param lonA Longitude of the first point.
 * @param latB Latitude of the second point.
 * @param lonB Longitude of the second point.
 * @return Bearing in degrees.
*/
double calculateBearing(long double latA, long double lonA, long double latB, long double lonB);

/**
 * Calculates the distance between two points.
 * @param latA Latitude of the first point.
 * @param lonA Longitude of the first point.
 * @param latB Latitude of the second point.
 * @param lonB Longitude of the second point.
 * @return Distance in meters.
*/
double calculateDistance(long double latA, long double lonA, long double latB, long double lonB);

#endif // __NAV_H
