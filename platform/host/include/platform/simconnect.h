#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "platform/types.h"

#if SIMCONNECT

typedef enum SCFlightControl {
    FCTRL_AIL,
    FCTRL_ELE,
    FCTRL_RUD,
    FCTRL_THR,
} SCFlightControl;

    #pragma pack(push, 1) // Pack structs for compatibility with SimConnect

// SimConnect (emulated) IMU data definition
typedef struct SC_IMU {
    f64 roll;         // deg
    f64 pitch;        // deg
    f64 yaw;          // deg
    f64 bodyAccel[3]; // [X, Y, Z], m/s^2; for internal use
    f64 gyro[3];      // [X, Y, Z], rad/s
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
 * Sets the requested flight control position, either in degrees (0-180) or percent (0-100)
 * @param fctrl the flight control to set
 * @param val the desired position
 * @return true if the data was sent successfully
 */
bool simconnect_set(SCFlightControl fctrl, f32 val);

/**
 * Gets a flight control position from SimConnect.
 * @param fctrl the flight control to get
 * @return the requested current flight control position, either in degrees (0-180) or percent (0-100)
 */
f32 simconnect_get(SCFlightControl fctrl);

/**
 * Closes an active connection to SimConnect if one exists.
 */
void simconnect_deinit();

extern SC_IMU scIMU;
extern SC_GPS scGPS;

// clang-format off
#ifdef __cplusplus
}
#endif
// clang-format on

#endif // SIMCONNECT
