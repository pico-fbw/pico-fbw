#include "../config.h"

#ifndef gps_h
#define gps_h

#define GPS_I2C i2c1

#ifdef GPS_ADAFRUIT_ULT

#endif

typedef struct gpsCoords {
    double lat;
    double lng;
} gpsCoords;

#endif
