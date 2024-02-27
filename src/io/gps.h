#pragma once

#include <stdbool.h>
#include "platform/int.h"

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

typedef bool (*gps_init_t)();
typedef void (*gps_update_t)();
typedef i32 (*gps_calibrate_alt_offset_t)(u32);
typedef bool (*gps_is_supported_t)();

typedef struct GPS {
    long double lat; // -90 to 90 deg. (Read-only)
    long double lng; // -180 to 180 deg. (Read-only)
    i32 alt; // MSL, ft. (Read-only)
    float speed; // Groundspeed, kts. (Read-only)
    float track; // True (NOT magnetic) heading, 0 to 360 deg. (Read-only)
    float pdop, hdop, vdop; // GPS DOP (dilution of precision) measurements for position, horizontal, and vertical (Read-only)
    i32 altOffset; // This is a positive value (basically where the GPS is MSL) or possibly zero if no calibration has been performed. (Read-only)
    bool altOffsetCalibrated; // (Read-only)
    /**
     * Initializes the GPS module.
     * @return true if successful, false if not.
    */
    gps_init_t init;
    /**
     * Obtains updated data from the GPS module and stores it in this GPS struct.
    */
    gps_update_t update;
    /**
     * Calibrates the altitude offset from the GPS.
     * @param num_samples the number of samples to take.
     * @return 0 if successful, -1 if a timeout occured, or -2 otherwise.
    */
    gps_calibrate_alt_offset_t calibrate_alt_offset;
    /**
     * @return whether or not the GPS sensor is supported in the current system configuration.
    */
    gps_is_supported_t is_supported;
} GPS;

extern GPS gps;
