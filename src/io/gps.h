#ifndef __GPS_H
#define __GPS_H

#include "../config.h"

// GPS uses uart1 because uart0 can be used for debugging
#define GPS_UART uart1

// NMEA-0183 specifies 4800/9600 baudrate, we use 4800 just to be safe
#define GPS_BAUD_RATE 4800

// TODO: naturally, once gps is done it needs to be added to wiki

/**
 * Initializes the GPS module.
 * @return true if successful, false if not.
*/
bool gps_init();

/**
 * Deinitializes the GPS module.
*/
void gps_deinit();

/**
 * Contains latitude and longitude coordinates, altitude, and speed from a connected GPS module when filled using gps_getData().
*/ 
typedef struct gpsData {
    double lat;
    double lng;
    int alt;
    float spd;
} gpsData;

/**
 * Gets the current coordinates from the GPS module.
 * @return an updated gpsData struct.
*/
gpsData gps_getData();

#endif // __GPS_H
