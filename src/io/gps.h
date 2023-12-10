#ifndef __GPS_H
#define __GPS_H

#include <stdbool.h>
#include "pico/types.h"

#define GPS_COMMAND_TYPE_MIN GPS_COMMAND_TYPE_NONE
typedef enum GPSCommandType {
    GPS_COMMAND_TYPE_NONE,
    GPS_COMMAND_TYPE_PMTK
    // What is the command type used for ublox modules?
} GPSCommandType;
#define GPS_COMMAND_TYPE_MAX GPS_COMMAND_TYPE_PMTK

#define GPS_UART uart1
#define GPS_UART_IRQ UART1_IRQ

// Maximum timeout between waiting for a response after a command is sent (in milleseconds)
// This actually needs to be quite long because the GPS may wait to send a response until its update rate comes around which is a bit slow
#define GPS_COMMAND_TIMEOUT_MS 1300

// These are the DOP thresholds to accept for safe flying, if any of the DOPs are larger than this the GPS will be considered unsafe
#define GPS_SAFE_PDOP_THRESHOLD 4
#define GPS_SAFE_HDOP_THRESHOLD 5
#define GPS_SAFE_VDOP_THRESHOLD 3

#define M_TO_FT 3.28084f // Meters to feet conversion constant

// TODO: gps needs to be added to wiki

typedef bool (*gps_init_t)();
typedef void (*gps_deinit_t)();
typedef void (*gps_update_t)();
typedef int (*gps_calibrateAltOffset_t)(uint);
typedef bool (*gps_isSupported_t)();

typedef struct GPS {
    long double lat; // -90 to 90 deg.
    long double lng; // -180 to 180 deg.
    int alt; // MSL, ft.
    float speed; // Groundspeed, kts.
    float track; // True (NOT magnetic) heading, 0 to 360 deg.
    float pdop, hdop, vdop; // GPS DOP (dilution of precision) measurements for position, horizontal, and vertical
    int altOffset; // This is a positive value (basically where the GPS is MSL) or possibly zero if no calibration has been performed.
    bool altOffset_calibrated;
    /**
     * Initializes the GPS module.
     * @return true if successful, false if not.
    */
    gps_init_t init;
    /**
     * Deinitializes the GPS module.
    */
    gps_deinit_t deinit;
    /**
     * Obtains updated data from the GPS module and stores it in this GPS struct.
    */
    gps_update_t update;
    /**
     * Calibrates the altitude offset from the GPS.
     * @param num_samples the number of samples to take.
     * @return 0 if successful, PICO_ERROR_TIMEOUT if a timeout occured, or PICO_ERROR_GENERIC otherwise.
    */
    gps_calibrateAltOffset_t calibrateAltOffset;
    /**
     * @return whether or not the GPS sensor is supported in the current system configuration.
    */
    gps_isSupported_t isSupported;
} GPS;

extern GPS gps;

#endif // __GPS_H
