#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "platform/types.h"

#if SIMCONNECT

/**
 * Initializes a connection to SimConnect.
 * @return true if the connection was successful
 */
bool simconnect_init();

/**
 * @return true if a connection to SimConnect is active
 */
bool simconnect_ready();

/**
 * Polls SimConnect for new messages.
 * Also updates scIMU and scGPS with the latest data.
 */
void simconnect_poll();

/**
 * Sets the aileron position in SimConnect.
 * @param ail the desired position in degrees (0-180)
 * @return true if the data was sent successfully
 */
bool simconnect_set_ail(f32 ail);

/**
 * Sets the elevator position in SimConnect.
 * @param ele the desired position in degrees (0-180)
 * @return true if the data was sent successfully
 */
bool simconnect_set_ele(f32 ele);

/**
 * Sets the rudder position in SimConnect.
 * @param rud the desired position in degrees (0-180)
 * @return true if the data was sent successfully
 */
bool simconnect_set_rud(f32 rud);

/**
 * Sets the throttle position in SimConnect.
 * @param thr the desired position in percent (0-100)
 * @return true if the data was sent successfully
 */
bool simconnect_set_thr(f32 thr);

/**
 * Closes an active connection to SimConnect if one exists.
 */
void simconnect_deinit();

    #pragma pack(push, 1) // Pack structs for compatibility with SimConnect
// SimConnect (emulated) IMU data definition
typedef struct SC_IMU {
    f64 roll;         // deg
    f64 pitch;        // deg
    f64 yaw;          // deg
    f64 bodyAccel[3]; // [X, Y, Z], m/s^2, mainly for internal use
    f64 gyro[3];      // [X, Y, Z], deg/s
    f32 alt;          // ft
    // Not populated by SimConnect, but computed from other data
    f64 accel[3]; // [X, Y, Z], g
} SC_IMU;

// SimConnect (emulated) GPS data definition
typedef struct SC_GPS {
    f64 lat;   // deg
    f64 lng;   // deg
    f32 alt;   // ft
    f32 speed; // kts
    f32 track; // deg
} SC_GPS;
    #pragma pack(pop)

extern SC_IMU scIMU;
extern SC_GPS scGPS;

    // clang-format off
#ifdef __cplusplus
}
#endif
// clang-format on

#endif // SIMCONNECT
