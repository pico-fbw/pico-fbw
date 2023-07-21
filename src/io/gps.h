#ifndef __GPS_H
#define __GPS_H

#include "../config.h"

#ifdef GPS_ENABLED

#define GPS_UART uart1 // GPS uses uart1 because uart0 can be used for debugging in some cases
#define GPS_UART_IRQ UART1_IRQ

// Timeout between waiting for characters during line reads (in microseconds)
// Too low and sentences will be chunked up into different function returns breaking parsing, too high and process time will be wasted
#define GPS_READ_TIMEOUT_US 2000

// Timeout between waiting for a response after a command is sent (in microseconds)
#define GPS_COMMAND_TIMEOUT_US 50000

// These are the DOP thresholds to accept for safe flying, if any of the DOPs are larger than this the GPS will be considered unsafe
#define GPS_SAFE_PDOP_THRESHOLD 3
#define GPS_SAFE_HDOP_THRESHOLD 5
#define GPS_SAFE_VDOP_THRESHOLD 2

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
 * Contains latitude and longitude coordinates, altitude, speed, and magnetic track from a connected GPS module when filled using gps_getData().
*/ 
typedef struct gpsData {
    double lat;
    double lng;
    int alt;
    float spd;
    float trk;
} gpsData;

/**
 * Gets the current coordinates from the GPS module.
 * @return an updated gpsData struct.
*/
gpsData gps_getData();

#endif // GPS_ENABLED

#endif // __GPS_H
