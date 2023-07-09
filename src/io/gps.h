#include "../config.h"

#ifndef __GPS_H
#define __GPS_H

#define GPS_I2C i2c1

#ifdef GPS_ADAFRUIT_ULT

#endif

/**
 * Initializes the GPS module.
 * @return 0 if success (correct module type was initialized and recognized),
 * PICO_ERROR_GENERIC if there was an I2C read/write failure,
 * PICO_ERROR_TIMEOUT if there was an I2C timeout, or
 * 1 if there was a general error
*/
int gps_init();

/**
 * Deinitializes the GPS module.
*/
void gps_deinit();

/**
 * Contains latitude and longitude coordinates, as well as current altitude and speed from a connected GPS module (when filled with its corresponding function).
*/
typedef struct gpsData {
    double lat;
    double lng;
    int_fast16_t alt;
    float speed;
} gpsData;

/**
 * Gets the current coordinates from the GPS module.
 * @return a gpsData struct containing current latitude and longitude coordinates, as well as current altitude and speed
*/
gpsData gps_getData();

#endif // __GPS_H
