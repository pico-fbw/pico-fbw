#pragma once

#include "platform/types.h"

/**
 * Calculates the bearing between two points.
 * @param latA Latitude of the first point.
 * @param lonA Longitude of the first point.
 * @param latB Latitude of the second point.
 * @param lonB Longitude of the second point.
 * @return Bearing in degrees.
 */
f64 calculate_bearing(f64 latA, f64 lonA, f64 latB, f64 lonB);

/**
 * Calculates the distance between two points.
 * @param latA Latitude of the first point.
 * @param lonA Longitude of the first point.
 * @param latB Latitude of the second point.
 * @param lonB Longitude of the second point.
 * @return Distance in meters.
 */
f64 calculate_distance(f64 latA, f64 lonA, f64 latB, f64 lonB);
