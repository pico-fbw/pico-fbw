#pragma once

/**
 * Calculates the bearing between two points.
 * @param latA Latitude of the first point.
 * @param lonA Longitude of the first point.
 * @param latB Latitude of the second point.
 * @param lonB Longitude of the second point.
 * @return Bearing in degrees.
*/
double calculate_bearing(long double latA, long double lonA, long double latB, long double lonB);

/**
 * Calculates the distance between two points.
 * @param latA Latitude of the first point.
 * @param lonA Longitude of the first point.
 * @param latB Latitude of the second point.
 * @param lonB Longitude of the second point.
 * @return Distance in meters.
*/
double calculate_distance(long double latA, long double lonA, long double latB, long double lonB);
