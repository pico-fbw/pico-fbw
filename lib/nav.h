#pragma once

#include "platform/types.h"

/**
 * Calculates the bearing between two points.
 * @param latA latitude of the first point
 * @param lngA longitude of the first point
 * @param latB latitude of the second point
 * @param lngB longitude of the second point
 * @return bearing in degrees
 */
f64 calculate_bearing(f64 latA, f64 lngA, f64 latB, f64 lngB);

/**
 * Calculates the distance between two points.
 * @param latA latitude of the first point
 * @param lngA longitude of the first point
 * @param latB latitude of the second point
 * @param lngB longitude of the second point
 * @return distance in meters
 */
f64 calculate_distance(f64 latA, f64 lngA, f64 latB, f64 lngB);
